#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "app_mqtt.h"
#include "app_pipeline.h"

static const char *TAG = "APP_MQTT";
static esp_mqtt_client_handle_t client = NULL;
static bool mqtt_connected = false;

// Example MQTT Broker URI (Using Eclipse's public testing broker)
// You will change this to your AWS IoT, ThingsBoard, or EMQX endpoint.
#define MQTT_BROKER_URI "mqtt://mqtt.eclipseprojects.io"
#define MQTT_TOPIC "iot_gateway/telemetry"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected to %s", MQTT_BROKER_URI);
        mqtt_connected = true;
        // TODO: We could subscribe to a config update topic here!
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "MQTT Disconnected");
        mqtt_connected = false;
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGD(TAG, "MQTT Published event, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT Received data on topic %.*s", event->topic_len, event->topic);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT Error event");
        break;
    default:
        break;
    }
}

// The core task that pulls from the unified pipeline and publishes JSON to MQTT
static void mqtt_publisher_task(void *arg)
{
    ESP_LOGI(TAG, "MQTT JSON Publisher Task Started");
    telemetry_msg_t msg;

    while (1) {
        // Wait forever for a new message to arrive in the queue from RS485/RS232/LoRa
        if (xQueueReceive(g_telemetry_queue, &msg, portMAX_DELAY) == pdTRUE) {
            
            if (!mqtt_connected) {
                // If offline, you would normally write `msg` to the SD Card for Store-and-Forward here.
                ESP_LOGW(TAG, "MQTT disconnected! Dropping message (Store-and-Forward not yet active).");
                continue;
            }

            // 1. Create a dynamic JSON object
            cJSON *json = cJSON_CreateObject();
            cJSON_AddNumberToObject(json, "timestamp", msg.timestamp);
            cJSON_AddNumberToObject(json, "device_id", msg.device_id);

            // Determine protocol source string
            const char *proto_str = "UNKNOWN";
            if (msg.source_protocol == PROTO_RS485) proto_str = "RS485";
            else if (msg.source_protocol == PROTO_RS232) proto_str = "RS232";
            else if (msg.source_protocol == PROTO_LORA) proto_str = "LORA";
            else if (msg.source_protocol == PROTO_CAN) proto_str = "CAN";

            cJSON_AddStringToObject(json, "protocol", proto_str);

            // 2. Map the unified union data into the JSON object
            if (msg.source_protocol == PROTO_RS485) {
                // Example: If it's RS485 Modbus, maybe we parsed it as a float value
                // (Note: Currently the RS485 task just dumps raw bytes, but this is the architecture)
                cJSON_AddNumberToObject(json, "value", msg.data.numeric_value);
                
                // For this PoC, since we pushed raw bytes from RS485, let's also dump them as hex
                char hex_str[65] = {0}; 
                for (int i = 0; i < msg.payload_length && i < 32; i++) {
                    sprintf(&hex_str[i * 2], "%02X", msg.data.payload[i]);
                }
                cJSON_AddStringToObject(json, "raw_payload", hex_str);

            } else {
                // For protocols like RS232, LoRa, or CAN, we convert the byte array to a Hex String.
                char hex_str[65] = {0}; 
                for (int i = 0; i < msg.payload_length && i < 32; i++) {
                    sprintf(&hex_str[i * 2], "%02X", msg.data.payload[i]);
                }
                cJSON_AddStringToObject(json, "raw_payload", hex_str);
            }

            // 3. Render JSON to a minimized string
            char *json_string = cJSON_PrintUnformatted(json);
            
            // 4. Publish to the Cloud
            int msg_id = esp_mqtt_client_publish(client, MQTT_TOPIC, json_string, 0, 1, 0);
            if (msg_id != -1) {
                ESP_LOGI(TAG, "Published Telemetry to Cloud: %s", json_string);
            } else {
                ESP_LOGE(TAG, "Failed to publish telemetry");
            }

            // 5. Cleanup memory instantly to avoid leaks
            free(json_string);
            cJSON_Delete(json);
        }
    }
}

esp_err_t app_mqtt_init(void)
{
    ESP_LOGI(TAG, "Initializing MQTT Client...");

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    // Spawn the FreeRTOS task that waits on the queue and publishes JSON
    xTaskCreate(mqtt_publisher_task, "mqtt_publisher_task", 4096, NULL, 4, NULL);

    return ESP_OK;
}

void app_mqtt_start(void)
{
    if (client) {
        ESP_LOGI(TAG, "Starting MQTT connection...");
        esp_mqtt_client_start(client);
    }
}
