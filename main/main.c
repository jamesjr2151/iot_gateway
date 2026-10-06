#include <stdio.h>
#include "esp_log.h"
#include "app_provisioning.h"
#include "app_sdcard.h"
#include "app_rs485.h"
#include "app_rs232.h"
#include "app_pipeline.h"
#include "app_mqtt.h"
#include "app_rtc.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ESP32-S3 PoC...");

    /* Start the provisioning module. This handles NVS init, WiFi setup, 
       and BLE provisioning if no credentials are found. */
    esp_err_t err = app_provisioning_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start provisioning module: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Application main loop started.");
    
    // Future features will be initialized here:
    // app_i2c_expander_init();
    
    ESP_LOGI(TAG, "Initializing SD Card...");
    if (app_sdcard_init() == ESP_OK) {
        // Only try to read the config if the SD card mounted successfully
        app_config_load_and_apply();
    }
    
    
    // Create the global telemetry queue before starting hardware tasks
    app_pipeline_init();
    
    // Initialize MQTT and start the JSON publisher task
    ESP_LOGI(TAG, "Initializing MQTT...");
    app_mqtt_init();
    
    ESP_LOGI(TAG, "Initializing RTC (DS3231)...");
    app_rtc_init();
    
    ESP_LOGI(TAG, "Initializing RS485...");
    app_rs485_init();
    
    ESP_LOGI(TAG, "Initializing RS232...");
    app_rs232_init();
    
    // Start the MQTT client connection
    app_mqtt_start();
}
