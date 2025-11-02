/**
 * @file presenter_mode_manager.c
 * @brief Presenter Mode Manager implementation
 */

#include "presenter_mode_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "general_config.h"
#include "lora_protocol.h"

static const char *TAG = "PRESENTER_MGR";
static SemaphoreHandle_t state_mutex = NULL;

esp_err_t presenter_mode_manager_init(void)
{
    state_mutex = xSemaphoreCreateMutex();
    if (!state_mutex) {
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "Presenter mode manager initialized");
    return ESP_OK;
}

esp_err_t presenter_mode_manager_handle_button(button_event_type_t button_type)
{
    if (!state_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    xSemaphoreTake(state_mutex, portMAX_DELAY);

    // Get configuration
    general_config_t config;
    general_config_get(&config);

    esp_err_t ret = ESP_OK;

    switch (button_type) {
        case BUTTON_EVENT_SHORT:
            ESP_LOGI(TAG, "Short press - sending Cursor Right");
#ifdef CONFIG_LORA_SEND_RELIABLE
            ret = lora_protocol_send_keyboard_reliable(config.slot_id, 0, 0x4F, 2000, 3);
#else
            ret = lora_protocol_send_keyboard(config.slot_id, 0, 0x4F);
#endif
            break;

        case BUTTON_EVENT_LONG:
            ESP_LOGI(TAG, "Long press - entering menu mode");
            // Long press enters menu mode, no LoRa transmission
            break;

        case BUTTON_EVENT_DOUBLE:
            ESP_LOGI(TAG, "Double press - sending Cursor Left");
#ifdef CONFIG_LORA_SEND_RELIABLE
            ret = lora_protocol_send_keyboard_reliable(config.slot_id, 0, 0x50, 2000, 3);
#else
            ret = lora_protocol_send_keyboard(config.slot_id, 0, 0x50);
#endif
            break;

        default:
            ESP_LOGW(TAG, "Unknown button type: %d", button_type);
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
