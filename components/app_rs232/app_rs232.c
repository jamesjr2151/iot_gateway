#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "app_pipeline.h"
#include "app_rtc.h"
#include "app_rs232.h"

static const char *TAG = "APP_RS232";

#define RS232_UART_PORT UART_NUM_2
#define BAUD_RATE       9600
#define BUF_SIZE        256
#define READ_TIMEOUT_MS 100

// The RS232 ingestion task
static void rs232_task(void *arg)
{
    ESP_LOGI(TAG, "RS232 Task Started. Listening for data...");
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    
    while (1) {
        // Read data from the RS232 UART
        int len = uart_read_bytes(RS232_UART_PORT, data, BUF_SIZE, READ_TIMEOUT_MS / portTICK_PERIOD_MS);
        
        if (len > 0) {
            ESP_LOGI(TAG, "Received %d bytes from RS232", len);
            ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
            // Build the unified telemetry message
            telemetry_msg_t msg = {
                .timestamp = app_rtc_get_timestamp_ms(),
                .source_protocol = PROTO_RS232,
                .device_id = 2, // Arbitrary ID for RS232 device
            };
            
            // Push the raw bytes into the union's payload array.
            msg.payload_length = (len > sizeof(msg.data.payload)) ? sizeof(msg.data.payload) : len;
            memcpy(msg.data.payload, data, msg.payload_length);

            // Push to the unified FreeRTOS queue
            if (g_telemetry_queue != NULL) {
                if (xQueueSend(g_telemetry_queue, &msg, pdMS_TO_TICKS(10)) == pdTRUE) {
                    ESP_LOGI(TAG, "Pushed to Unified Pipeline -> [RS232]");
                } else {
                    ESP_LOGW(TAG, "Unified pipeline queue FULL! Dropping RS232 message.");
                }
            }
        }
        
        // Polling delay
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    free(data);
    vTaskDelete(NULL);
}

esp_err_t app_rs232_init(void)
{
    ESP_LOGI(TAG, "Initializing RS232 UART...");

    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    esp_err_t err = uart_driver_install(RS232_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver");
        return err;
    }

    // Configure UART parameters
    uart_param_config(RS232_UART_PORT, &uart_config);

    // Set UART pins (TX, RX only)
    uart_set_pin(RS232_UART_PORT, RS232_TXD_PIN, RS232_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
    // Spawn the FreeRTOS task to handle the incoming RS232 data continuously
    xTaskCreate(rs232_task, "rs232_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}
