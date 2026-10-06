#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Saves a JSON telemetry string to the offline SD card buffer.
 * 
 * @param json_string The rendered JSON string to save.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_store_forward_save(const char *json_string);

/**
 * @brief Initializes the background task that checks for offline files 
 *        and forwards them to MQTT when the network is restored.
 */
void app_store_forward_init(void);

#ifdef __cplusplus
}
#endif
