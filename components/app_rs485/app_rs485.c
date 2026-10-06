#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "app_rs485.h"

static const char *TAG = "APP_RS485";

#define RS485_UART_PORT UART_NUM_1
#define BAUD_RATE       9600
#define BUF_SIZE        256
#define READ_TIMEOUT_MS 100

// The RS485 ingestion task
static void rs485_task(void *arg)
{
    ESP_LOGI(TAG, "RS485 Task Started. Listening for data...");
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    
    while (1) {
        // Read data from the RS485 UART
        int len = uart_read_bytes(RS485_UART_PORT, data, BUF_SIZE, READ_TIMEOUT_MS / portTICK_PERIOD_MS);
        
        if (len > 0) {
            ESP_LOGI(TAG, "Received %d bytes from RS485", len);
            ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);
            
            /* 
             * TODO: Unified Data Pipeline Integration
             * 1. Parse the Modbus/Raw data here.
             * 2. Convert it into the `telemetry_msg_t` struct.
             * 3. Push the struct into the FreeRTOS JSON/MQTT queue!
             */
        }
        
        // Polling delay
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
    free(data);
    vTaskDelete(NULL);
}

esp_err_t app_rs485_init(void)
{
    ESP_LOGI(TAG, "Initializing RS485 UART...");

    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install UART driver
    esp_err_t err = uart_driver_install(RS485_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver");
        return err;
    }

    // Configure UART parameters
    uart_param_config(RS485_UART_PORT, &uart_config);

    // Set UART pins (TX, RX, RTS for RE/DE, CTS not used)
    uart_set_pin(RS485_UART_PORT, RS485_TXD_PIN, RS485_RXD_PIN, RS485_RTS_PIN, UART_PIN_NO_CHANGE);

    // Set RS485 half duplex mode
    // This automatically toggles the RTS pin high when transmitting and low when receiving!
    uart_set_mode(RS485_UART_PORT, UART_MODE_RS485_HALF_DUPLEX);
    
    // Spawn the FreeRTOS task to handle the incoming RS485 data continuously
    xTaskCreate(rs485_task, "rs485_task", 4096, NULL, 5, NULL);

    return ESP_OK;
}
