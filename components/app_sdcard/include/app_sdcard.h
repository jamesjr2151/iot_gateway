#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default SPI pins for SD Card (Change these to match your devkit wiring)
#define SD_PIN_NUM_MISO  13
#define SD_PIN_NUM_MOSI  11
#define SD_PIN_NUM_CLK   12
#define SD_PIN_NUM_CS    10

/**
 * @brief Initialize the SD Card via SPI and mount the FAT filesystem.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_sdcard_init(void);

/**
 * @brief Unmount the SD Card and free resources.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_sdcard_deinit(void);

#ifdef __cplusplus
}
#endif
