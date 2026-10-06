#ifndef APP_PROVISIONING_H
#define APP_PROVISIONING_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the application provisioning.
 * 
 * This function handles NVS initialization, TCP/IP stack setup, 
 * and event loop creation. It checks if the device has WiFi credentials 
 * saved. If not, it falls back to BLE provisioning.
 * 
 * @return ESP_OK on success, or an error code
 */
esp_err_t app_provisioning_start(void);

#ifdef __cplusplus
}
#endif

#endif // APP_PROVISIONING_H
