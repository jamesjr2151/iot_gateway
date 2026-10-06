#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the MQTT client and spawn the JSON publisher task.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_mqtt_init(void);

/**
 * @brief Start connecting the MQTT client to the broker.
 */
void app_mqtt_start(void);

#ifdef __cplusplus
}
#endif
