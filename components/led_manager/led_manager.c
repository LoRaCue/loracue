/**
 * @file led_manager.c
 * @brief LED Manager implementation with solid, fading and blinking patterns
 */

#include "led_manager.h"
#include "bsp.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <math.h>

static const char *TAG = "LED_MANAGER";

// LEDC configuration for PWM fading
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY 5000
#define LEDC_MAX_DUTY ((1 << LEDC_DUTY_RES) - 1)

// State management
static led_pattern_t current_pattern = LED_PATTERN_OFF;
static TaskHandle_t fade_task_handle = NULL;
static uint32_t fade_period_ms       = 2000;
static bool button_feedback_active   = false;

// Forward declaration
static void fade_task(void *pvParameters);

esp_err_t led_manager_init(void)
{
    ESP_LOGI(TAG, "Initializing LED manager");

    // Configure LEDC for PWM fading
    ledc_timer_config_t ledc_timer = {.speed_mode      = LEDC_MODE,
                                      .timer_num       = LEDC_TIMER,
                                      .duty_resolution = LEDC_DUTY_RES,
                                      .freq_hz         = LEDC_FREQUENCY,
                                      .clk_cfg         = LEDC_AUTO_CLK};
    esp_err_t ret                  = ledc_timer_config(&ledc_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC timer: %s", esp_err_to_name(ret));
        return ret;
    }

    ledc_channel_config_t ledc_channel = {.speed_mode = LEDC_MODE,
                                          .channel    = LEDC_CHANNEL,
                                          .timer_sel  = LEDC_TIMER,
                                          .intr_type  = LEDC_INTR_DISABLE,
                                          .gpio_num   = 35, // STATUS_LED_PIN
                                          .duty       = 0,
                                          .hpoint     = 0};
    ret                                = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LEDC channel: %s", esp_err_to_name(ret));
        return ret;
    }

    current_pattern = LED_PATTERN_OFF;
    ESP_LOGI(TAG, "LED manager initialized");
    return ESP_OK;
}

esp_err_t led_manager_solid(bool on)
{
    ESP_LOGD(TAG, "Setting LED solid: %s", on ? "ON" : "OFF");

    // Stop any running patterns
    led_manager_stop();

    current_pattern = on ? LED_PATTERN_SOLID : LED_PATTERN_OFF;

    // Set LED via LEDC PWM
    uint32_t duty = on ? LEDC_MAX_DUTY : 0;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    return ESP_OK;
}

esp_err_t led_manager_blink(uint32_t period_ms, uint8_t duty_percent)
{
    // Simplified - not needed for current requirements
    return led_manager_solid(true);
}

esp_err_t led_manager_fade(uint32_t period_ms)
{
    ESP_LOGD(TAG, "Starting LED fade: %dms period", period_ms);

    // Stop any running patterns
    led_manager_stop();

    fade_period_ms  = period_ms;
    current_pattern = LED_PATTERN_FADE;

    // Create fade task
    BaseType_t ret = xTaskCreate(fade_task, "led_fade", 2048, NULL, 5, &fade_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create fade task");
        current_pattern = LED_PATTERN_OFF;
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

esp_err_t led_manager_stop(void)
{
    ESP_LOGD(TAG, "Stopping LED patterns");

    // Stop fade task
    if (fade_task_handle != NULL) {
        vTaskDelete(fade_task_handle);
        fade_task_handle = NULL;
    }

    // Turn off LED
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

    current_pattern = LED_PATTERN_OFF;
    return ESP_OK;
}

led_pattern_t led_manager_get_pattern(void)
{
    return current_pattern;
}

static void fade_task(void *pvParameters)
{
    const uint32_t step_ms         = 20; // 50Hz update rate
    const uint32_t steps_per_cycle = fade_period_ms / step_ms;
    uint32_t step                  = 0;

    ESP_LOGD(TAG, "Fade task started: %d steps per cycle", steps_per_cycle);

    while (current_pattern == LED_PATTERN_FADE) {
        // Calculate sine wave position (0 to 2π)
        float angle = (2.0f * M_PI * step) / steps_per_cycle;

        // Generate sine wave (0 to 1)
        float sine_val = (sinf(angle) + 1.0f) / 2.0f;

        // Convert to duty cycle (override to 0 if button feedback active)
        uint32_t duty = button_feedback_active ? 0 : (uint32_t)(sine_val * LEDC_MAX_DUTY);

        // Set PWM duty
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

        step = (step + 1) % steps_per_cycle;
        vTaskDelay(pdMS_TO_TICKS(step_ms));
    }

    ESP_LOGD(TAG, "Fade task ended");
    fade_task_handle = NULL;
    vTaskDelete(NULL);
}

esp_err_t led_manager_button_feedback(bool active)
{
    button_feedback_active = active;
    
    // For non-fade patterns, immediately turn LED off/on
    if (active && current_pattern != LED_PATTERN_FADE) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    } else if (!active && current_pattern == LED_PATTERN_SOLID) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_MAX_DUTY);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
    }
    
    return ESP_OK;
}
