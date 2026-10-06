#pragma once

#include "esp_err.h"
#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Default I2C pins for DS3231 (adjust as needed for your specific devkit)
#define I2C_MASTER_SDA_PIN  21
#define I2C_MASTER_SCL_PIN  22

/**
 * @brief Initialize the I2C bus and the DS3231 RTC module.
 *        This will automatically read the RTC time and sync the ESP32's internal system clock.
 * 
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t app_rtc_init(void);

/**
 * @brief Get the current epoch timestamp in milliseconds (uses the synced system clock).
 * 
 * @return uint64_t Epoch time in milliseconds.
 */
uint64_t app_rtc_get_timestamp_ms(void);

#ifdef __cplusplus
}
#endif
