#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_store_forward.h"
#include "app_mqtt.h"

static const char *TAG = "STORE_FORWARD";
#define OFFLINE_FILE "/sdcard/offline.jsonl"
#define PROCESSING_FILE "/sdcard/processing.jsonl"

esp_err_t app_store_forward_save(const char *json_string)
{
    // Append the JSON string as a new line to the offline file
    FILE *f = fopen(OFFLINE_FILE, "a");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open offline storage file. Is SD Card mounted?");
        return ESP_FAIL;
    }
    
    fprintf(f, "%s\n", json_string);
    fclose(f);
    
    ESP_LOGI(TAG, "Saved message to offline storage.");
    return ESP_OK;
}

static void store_forward_task(void *arg)
{
    char line[1024]; // Buffer for a single JSON line
    
    while (1) {
        // Wake up and check every 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
        
        // Only attempt forwarding if MQTT is actually connected
        if (app_mqtt_is_connected()) {
            
            struct stat st;
            // Check if the offline file exists
            if (stat(OFFLINE_FILE, &st) == 0 && st.st_size > 0) {
                ESP_LOGI(TAG, "Offline data found! Attempting to forward to cloud...");
                
                // Rename to a processing file. This prevents the primary data ingress tasks 
                // from appending to this exact file while we are actively reading and deleting it.
                rename(OFFLINE_FILE, PROCESSING_FILE);
                
                FILE *f = fopen(PROCESSING_FILE, "r");
                if (f != NULL) {
                    bool abort_processing = false;
                    
                    // Read line by line (each line is one full JSON payload)
                    while (fgets(line, sizeof(line), f) != NULL) {
                        
                        // Strip the trailing newline character
                        line[strcspn(line, "\n")] = 0;
                        
                        if (strlen(line) > 0) {
                            // Try to publish it
                            if (app_mqtt_publish(line) != -1) {
                                ESP_LOGD(TAG, "Forwarded offline msg: %s", line);
                                // Small delay to avoid flooding the MQTT broker / network stack
                                vTaskDelay(pdMS_TO_TICKS(100)); 
                            } else {
                                // If publish returned -1, we lost connection mid-forward!
                                ESP_LOGW(TAG, "MQTT disconnected during forward! Aborting.");
                                abort_processing = true;
                                break;
                            }
                        }
                    }
                    fclose(f);
                    
                    if (!abort_processing) {
                        // We successfully forwarded every line in the file. Delete it!
                        remove(PROCESSING_FILE);
                        ESP_LOGI(TAG, "All offline data successfully forwarded and cleared!");
                    } else {
                        // We disconnected midway. 
                        // For this PoC, we will simply rename the file back so we can retry the whole file later.
                        // (Note: In production, you would ideally track byte offsets to avoid duplicate uploads).
                        rename(PROCESSING_FILE, OFFLINE_FILE);
                    }
                }
            }
        }
    }
}

void app_store_forward_init(void)
{
    ESP_LOGI(TAG, "Initializing Store-and-Forward background task...");
    xTaskCreate(store_forward_task, "store_forward_task", 4096, NULL, 3, NULL);
}
