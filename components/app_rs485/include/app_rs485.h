#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Default RS485 pins (Change these based on your specific RS485 transceiver wiring)
#define RS485_TXD_PIN 17
#define RS485_RXD_PIN 16
#define RS485_RTS_PIN 18 // RTS is used for the RE/DE (Read/Drive Enable) control pin on the RS485 chip

/**
 * @brief Initialize the UART for RS485 Half-Duplex communication.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_rs485_init(void);

#ifdef __cplusplus
}
#endif
