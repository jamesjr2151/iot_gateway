#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default RS232 pins (Change these based on your RS232 transceiver wiring)
#define RS232_TXD_PIN 4
#define RS232_RXD_PIN 5

/**
 * @brief Initialize the UART for RS232 communication.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_rs232_init(void);

#ifdef __cplusplus
}
#endif
