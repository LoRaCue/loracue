/**
 * @file presenter_mode_manager.c
 * @brief Presenter Mode Manager implementation
 */

#include "presenter_mode_manager.h"
#include "common_types.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "general_config.h"
#include "lora_protocol.h"

static const char *TAG = "PRESENTER_MGR";

static SemaphoreHandle_t state_mutex = NULL;

// LoRa reliable transmission constants

#define LORA_RELIABLE_TIMEOUT_MS 2000

#define LORA_RELIABLE_MAX_RETRIES 3

// HID Keycodes (Standard Usage ID)

#define HID_KEY_ARROW_RIGHT 0x4F

#define HID_KEY_ARROW_LEFT 0x50

esp_err_t presenter_mode_manager_init(void)

{

    CREATE_MUTEX_OR_FAIL(state_mutex);

    ESP_LOGI(TAG, "Presenter mode manager initialized");

    return ESP_OK;
}

esp_err_t presenter_mode_manager_handle_input(input_event_t event)

{

    if (!state_mutex) {

        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    // Get configuration

    general_config_t config;

    general_config_get(&config);

    esp_err_t ret = ESP_OK;

    switch (event) {

        // Alpha: short press = next slide
        case INPUT_EVENT_NEXT_SHORT:
        // Alpha+: NEXT button short press = next slide (same event)

            ESP_LOGI(TAG, "Next slide - sending Cursor Right");

#ifdef CONFIG_LORACUE_LORA_SEND_RELIABLE

            ret = lora_protocol_send_keyboard_reliable(config.slot_id, 0, HID_KEY_ARROW_RIGHT,

                                                       LORA_RELIABLE_TIMEOUT_MS,

                                                       LORA_RELIABLE_MAX_RETRIES);

#else

            ret = lora_protocol_send_keyboard(config.slot_id, 0, HID_KEY_ARROW_RIGHT);

#endif

            break;

        // Alpha: long press = menu (handled by UI)
        case INPUT_EVENT_NEXT_LONG:
        // Alpha+: encoder button = menu (handled by UI)
        case INPUT_EVENT_ENCODER_BUTTON_SHORT:

            ESP_LOGI(TAG, "Menu button - no LoRa transmission");

            break;

        // Alpha: double press = prev slide
        case INPUT_EVENT_NEXT_DOUBLE:
        // Alpha+: PREV button short press = prev slide
        case INPUT_EVENT_PREV_SHORT:

            ESP_LOGI(TAG, "Previous slide - sending Cursor Left");

#ifdef CONFIG_LORACUE_LORA_SEND_RELIABLE

            ret = lora_protocol_send_keyboard_reliable(config.slot_id, 0, HID_KEY_ARROW_LEFT,

                                                       LORA_RELIABLE_TIMEOUT_MS,

                                                       LORA_RELIABLE_MAX_RETRIES);

#else

            ret = lora_protocol_send_keyboard(config.slot_id, 0, HID_KEY_ARROW_LEFT);

#endif

            break;

        default:
            ESP_LOGW(TAG, "Unknown input event: %d", event);
            ret = ESP_ERR_INVALID_ARG;
            break;
    }

    xSemaphoreGive(state_mutex);
    return ret;
}

void presenter_mode_manager_deinit(void)
{
    if (state_mutex) {
        vSemaphoreDelete(state_mutex);
        state_mutex = NULL;
    }
    ESP_LOGI(TAG, "Presenter mode manager deinitialized");
}
