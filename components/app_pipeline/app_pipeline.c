#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include "app_pipeline.h"

static const char *TAG = "APP_PIPELINE";

// Definition of the global queue handle
QueueHandle_t g_telemetry_queue = NULL;

void app_pipeline_init(void) 
{
    // Create a queue that can hold 50 unified telemetry messages
    g_telemetry_queue = xQueueCreate(50, sizeof(telemetry_msg_t));
    
    if (g_telemetry_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create telemetry queue!");
    } else {
        ESP_LOGI(TAG, "Unified Telemetry queue created successfully.");
    }
}

void app_config_load_and_apply(void) 
{
    ESP_LOGI(TAG, "Loading config.json from SD Card...");
    
    FILE *f = fopen("/sdcard/config.json", "r");
    if (f == NULL) {
        ESP_LOGW(TAG, "Failed to open config.json. Make sure it exists on the SD card!");
        return;
    }

    // Determine file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (fsize <= 0) {
        ESP_LOGE(TAG, "config.json is empty.");
        fclose(f);
        return;
    }

    // Allocate memory for the file content (PSRAM is preferred for large files, but standard malloc works here)
    char *buffer = malloc(fsize + 1);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for config file.");
        fclose(f);
        return;
    }

    fread(buffer, 1, fsize, f);
    fclose(f);
    buffer[fsize] = 0; // Null terminate the string

    // Parse the JSON
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error parsing JSON before: %s\n", error_ptr);
        }
        free(buffer);
        return;
    }

    ESP_LOGI(TAG, "Successfully parsed config.json. Applying dynamic flags...");

    // Extract active protocols
    cJSON *active_protocols = cJSON_GetObjectItemCaseSensitive(json, "active_protocols");
    if (active_protocols) {
        
        cJSON *rs485 = cJSON_GetObjectItemCaseSensitive(active_protocols, "rs485");
        if (cJSON_IsTrue(rs485)) {
            ESP_LOGI(TAG, "--> Dynamic Flag: RS485 is ENABLED.");
            // Later we can trigger app_rs485_init() from here!
        } else {
            ESP_LOGI(TAG, "--> Dynamic Flag: RS485 is DISABLED. Hardware will stay asleep.");
        }
        
        cJSON *lora = cJSON_GetObjectItemCaseSensitive(active_protocols, "lora");
        if (cJSON_IsTrue(lora)) {
            ESP_LOGI(TAG, "--> Dynamic Flag: LoRa is ENABLED.");
        }
        
        cJSON *can = cJSON_GetObjectItemCaseSensitive(active_protocols, "can");
        if (cJSON_IsTrue(can)) {
            ESP_LOGI(TAG, "--> Dynamic Flag: CAN (TWAI) is ENABLED.");
        }
    }

    // Cleanup memory
    cJSON_Delete(json);
    free(buffer);
}
