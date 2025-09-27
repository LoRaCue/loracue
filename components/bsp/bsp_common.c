#include "bsp.h"
#include "esp_log.h"

static const char *TAG = "BSP";

// Forward declaration of board-specific interfaces
extern const bsp_interface_t heltec_v3_interface;

const bsp_interface_t* bsp_get_interface(void)
{
    // For now, default to Heltec V3
    // Future: Add board detection or menuconfig selection
    return &heltec_v3_interface;
}

esp_err_t bsp_init(void)
{
    ESP_LOGI(TAG, "Initializing Board Support Package");
    
    const bsp_interface_t* bsp = bsp_get_interface();
    if (bsp && bsp->init) {
        return bsp->init();
    }
    
    return ESP_ERR_NOT_SUPPORTED;
}
