#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "app_rtc.h"

static const char *TAG = "APP_RTC";

#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_MASTER_FREQ_HZ  100000
#define DS3231_ADDR         0x68

// Helper: Convert BCD to Decimal
static uint8_t bcd2dec(uint8_t val) {
    return (val >> 4) * 10 + (val & 0x0F);
}

// Helper: Convert Decimal to BCD
static uint8_t dec2bcd(uint8_t val) {
    return ((val / 10) << 4) + (val % 10);
}

// Sync ESP32 system clock with DS3231
static esp_err_t ds3231_sync_to_system(void)
{
    uint8_t data[7];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Start reading at register 0 (Seconds)
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (DS3231_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, data + 6, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from DS3231");
        return ret;
    }

    struct tm timeinfo = {0};
    timeinfo.tm_sec  = bcd2dec(data[0]);
    timeinfo.tm_min  = bcd2dec(data[1]);
    timeinfo.tm_hour = bcd2dec(data[2] & 0x3F); // 24-hour mode
    timeinfo.tm_wday = bcd2dec(data[3]) - 1;
    timeinfo.tm_mday = bcd2dec(data[4]);
    timeinfo.tm_mon  = bcd2dec(data[5] & 0x1F) - 1;
    timeinfo.tm_year = bcd2dec(data[6]) + 100; // Since 1900

    time_t t = mktime(&timeinfo);
    
    struct timeval now = { .tv_sec = t, .tv_usec = 0 };
    settimeofday(&now, NULL);
    
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "System time synced from DS3231: %s", strftime_buf);

    return ESP_OK;
}

esp_err_t app_rtc_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C and DS3231...");

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_PIN,
        .scl_io_num = I2C_MASTER_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) return err;

    err = i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);
    if (err != ESP_OK) return err;

    // Immediately pull the time from the RTC chip and update the ESP32 CPU time
    return ds3231_sync_to_system();
}

uint64_t app_rtc_get_timestamp_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}
