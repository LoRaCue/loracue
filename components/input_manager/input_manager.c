/**
 * @file input_manager.c
 * @brief Unified input management implementation
 *
 * @copyright Copyright (c) 2025 LoRaCue Project
 * @license GPL-3.0
 */

#include "input_manager.h"
#include "bsp.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "led_manager.h"
#include "lv_port_disp.h"
#include "power_mgmt.h"
#include "sdkconfig.h"
#include "system_events.h"

#if CONFIG_LORACUE_INPUT_HAS_ENCODER
#include "encoder.h"
#endif

static const char *TAG = "INPUT_MGR";

// Task configuration
#define INPUT_TASK_STACK_SIZE 4096
#define INPUT_TASK_PRIORITY 5
#define INPUT_QUEUE_SIZE 10
#define INPUT_POLL_INTERVAL_MS 10
#define ENCODER_QUEUE_SIZE 4

// Timing from Kconfig
#define DEBOUNCE_MS CONFIG_LORACUE_INPUT_DEBOUNCE_MS
#define LONG_PRESS_MS CONFIG_LORACUE_INPUT_LONG_PRESS_MS
#define DOUBLE_PRESS_MS CONFIG_LORACUE_INPUT_DOUBLE_PRESS_MS

// State tracking
static input_callback_t s_callback = NULL;
static TaskHandle_t s_task_handle  = NULL;
static QueueHandle_t s_event_queue = NULL;
static bool s_initialized          = false;

// Button state tracking
typedef struct {
    bool pressed;
    uint32_t press_start_ms;
    uint32_t last_release_ms;
    bool long_sent;
    uint8_t click_count;
} button_state_t;

#if CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
static button_state_t s_prev_btn = {0};
static button_state_t s_next_btn = {0};
#else
static button_state_t s_btn = {0};
#endif

#if CONFIG_LORACUE_INPUT_HAS_ENCODER
// Encoder state
static rotary_encoder_t s_encoder      = {0};
static QueueHandle_t s_encoder_queue   = NULL;
static bool s_encoder_btn_pressed      = false;
static uint32_t s_encoder_btn_start_ms = 0;
#endif

static const char *event_to_string(input_event_t event)
{
    switch (event) {
        case INPUT_EVENT_NEXT_SHORT:
            return "NEXT_SHORT";
        case INPUT_EVENT_NEXT_LONG:
            return "NEXT_LONG";
        case INPUT_EVENT_NEXT_DOUBLE:
            return "NEXT_DOUBLE";
        case INPUT_EVENT_PREV_SHORT:
            return "PREV_SHORT";
        case INPUT_EVENT_PREV_LONG:
            return "PREV_LONG";
        case INPUT_EVENT_ENCODER_CW:
            return "ENCODER_CW";
        case INPUT_EVENT_ENCODER_CCW:
            return "ENCODER_CCW";
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:
            return "ENCODER_BTN_SHORT";
        case INPUT_EVENT_ENCODER_BUTTON_LONG:
            return "ENCODER_BTN_LONG";
        default:
            return "UNKNOWN";
    }
}

static inline void post_event(input_event_t event)
{
    ESP_LOGI(TAG, "Event: %s", event_to_string(event));
    xQueueSend(s_event_queue, &event, 0);
}

static void handle_button(button_state_t *btn, bool pressed, uint32_t now, input_event_t short_evt,
                          input_event_t long_evt, input_event_t double_evt)
{
    if (pressed && !btn->pressed) {
        btn->pressed        = true;
        btn->press_start_ms = now;
        btn->long_sent      = false;
        led_manager_button_feedback(true);
        display_safe_wake();
        power_mgmt_update_activity();
    } else if (pressed && !btn->long_sent && (now - btn->press_start_ms) >= LONG_PRESS_MS) {
        post_event(long_evt);
        btn->long_sent   = true;
        btn->click_count = 0;
    } else if (!pressed && btn->pressed) {
        btn->pressed      = false;
        uint32_t duration = now - btn->press_start_ms;
        led_manager_button_feedback(false);

        if (btn->long_sent) {
            btn->click_count = 0;
        } else if (duration < LONG_PRESS_MS) {
            btn->click_count++;
            btn->last_release_ms = now;
        }
    }

    // Double-click detection
    if (!btn->pressed && btn->click_count > 0) {
        uint32_t since_release = now - btn->last_release_ms;
        if (btn->click_count == 2) {
            post_event(double_evt);
            btn->click_count = 0;
        } else if (since_release >= DOUBLE_PRESS_MS) {
            post_event(short_evt);
            btn->click_count = 0;
        }
    }
}

#if CONFIG_LORACUE_INPUT_HAS_ENCODER
static void handle_encoder(uint32_t now)
{
    // Read encoder events from queue
    rotary_encoder_event_t enc_event;
    while (xQueueReceive(s_encoder_queue, &enc_event, 0) == pdTRUE) {
        if (enc_event.type == RE_ET_CHANGED) {
            if (enc_event.diff > 0) {
                post_event(INPUT_EVENT_ENCODER_CW);
            } else if (enc_event.diff < 0) {
                post_event(INPUT_EVENT_ENCODER_CCW);
            }
        }
    }

    // Encoder button (handled separately)
    bool btn_pressed      = !gpio_get_level(bsp_get_encoder_btn_gpio());
    static bool long_sent = false;

    if (btn_pressed && !s_encoder_btn_pressed) {
        s_encoder_btn_pressed  = true;
        s_encoder_btn_start_ms = now;
        long_sent              = false;
    } else if (btn_pressed && !long_sent && (now - s_encoder_btn_start_ms) >= LONG_PRESS_MS) {
        post_event(INPUT_EVENT_ENCODER_BUTTON_LONG);
        long_sent = true;
    } else if (!btn_pressed && s_encoder_btn_pressed) {
        s_encoder_btn_pressed = false;
        uint32_t duration     = now - s_encoder_btn_start_ms;
        if (!long_sent && duration >= DEBOUNCE_MS && duration < LONG_PRESS_MS) {
            post_event(INPUT_EVENT_ENCODER_BUTTON_SHORT);
        }
    }
}
#endif

static void input_task(void *arg)
{
    ESP_LOGI(TAG, "Input manager task started");

    input_event_t event;
    while (1) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;

#if CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
        // Alpha+: Dual buttons
        bool prev_pressed = !gpio_get_level(bsp_get_button_prev_gpio());
        bool next_pressed = !gpio_get_level(bsp_get_button_next_gpio());
        handle_button(&s_prev_btn, prev_pressed, now, INPUT_EVENT_PREV_SHORT, INPUT_EVENT_PREV_LONG,
                      INPUT_EVENT_NEXT_DOUBLE);
        handle_button(&s_next_btn, next_pressed, now, INPUT_EVENT_NEXT_SHORT, INPUT_EVENT_NEXT_LONG,
                      INPUT_EVENT_NEXT_DOUBLE);
#else
        // Alpha: Single button
        bool btn_pressed = bsp_read_button(BSP_BUTTON_NEXT);
        handle_button(&s_btn, btn_pressed, now, INPUT_EVENT_NEXT_SHORT, INPUT_EVENT_NEXT_LONG, INPUT_EVENT_NEXT_DOUBLE);
#endif

#if CONFIG_LORACUE_INPUT_HAS_ENCODER
        handle_encoder(now);
#endif

        // Process events
        if (xQueueReceive(s_event_queue, &event, pdMS_TO_TICKS(INPUT_POLL_INTERVAL_MS)) == pdTRUE) {
            if (s_callback) {
                s_callback(event);
            }
        }
    }
}

esp_err_t input_manager_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing input manager");

    s_event_queue = xQueueCreate(INPUT_QUEUE_SIZE, sizeof(input_event_t));
    if (!s_event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

#if CONFIG_LORACUE_INPUT_HAS_DUAL_BUTTONS
    // Configure PREV button
    gpio_config_t prev_cfg = {
        .pin_bit_mask = (1ULL << bsp_get_button_prev_gpio()),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&prev_cfg);

    // Configure NEXT button
    gpio_config_t next_cfg = {
        .pin_bit_mask = (1ULL << bsp_get_button_next_gpio()),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&next_cfg);
#endif

#if CONFIG_LORACUE_INPUT_HAS_ENCODER
    // Initialize encoder library with queue
    s_encoder_queue = xQueueCreate(ENCODER_QUEUE_SIZE, sizeof(rotary_encoder_event_t));
    if (!s_encoder_queue) {
        ESP_LOGE(TAG, "Failed to create encoder queue");
        return ESP_ERR_NO_MEM;
    }
    ESP_ERROR_CHECK(rotary_encoder_init(s_encoder_queue));

    // Configure encoder descriptor
    s_encoder.pin_a   = bsp_get_encoder_clk_gpio();
    s_encoder.pin_b   = bsp_get_encoder_dt_gpio();
    s_encoder.pin_btn = GPIO_NUM_MAX; // We handle button separately

    // Add encoder
    ESP_ERROR_CHECK(rotary_encoder_add(&s_encoder));

    // Configure encoder button GPIO
    gpio_config_t enc_btn_cfg = {
        .pin_bit_mask = (1ULL << bsp_get_encoder_btn_gpio()),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&enc_btn_cfg);
#endif

    s_initialized = true;
    ESP_LOGI(TAG, "Input manager initialized");
    return ESP_OK;
}

esp_err_t input_manager_register_callback(input_callback_t callback)
{
    if (!callback) {
        return ESP_ERR_INVALID_ARG;
    }

    s_callback = callback;
    ESP_LOGI(TAG, "Callback registered");
    return ESP_OK;
}

esp_err_t input_manager_start(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "Not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (s_task_handle) {
        ESP_LOGW(TAG, "Already started");
        return ESP_OK;
    }

    BaseType_t ret =
        xTaskCreate(input_task, "input_mgr", INPUT_TASK_STACK_SIZE, NULL, INPUT_TASK_PRIORITY, &s_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Input manager started");
    return ESP_OK;
}
