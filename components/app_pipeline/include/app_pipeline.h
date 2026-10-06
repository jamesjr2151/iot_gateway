#pragma once

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#ifdef __cplusplus
extern "C" {
#endif

// Enum for all supported protocols
typedef enum {
    PROTO_RS485,
    PROTO_RS232,
    PROTO_CAN,
    PROTO_LORA,
    PROTO_ZIGBEE,
    PROTO_UNKNOWN
} protocol_type_t;

// The Universal Telemetry Struct
typedef struct {
    uint64_t timestamp;
    protocol_type_t source_protocol;
    uint16_t device_id;
    
    // Union to handle different data shapes in the exact same memory footprint
    union {
        float numeric_value;          // E.g., a temperature from Modbus
        uint8_t payload[32];          // E.g., a raw byte packet from LoRa or CAN
    } data;
    
    uint8_t payload_length;           // Important when reading from the byte payload
} telemetry_msg_t;

// Global FreeRTOS Queue Handle that all protocols push to
extern QueueHandle_t g_telemetry_queue;

/**
 * @brief Initialize the telemetry pipeline (creates the queue).
 */
void app_pipeline_init(void);

/**
 * @brief Parse the config.json from the SD card and apply dynamic flags.
 */
void app_config_load_and_apply(void);

#ifdef __cplusplus
}
#endif
