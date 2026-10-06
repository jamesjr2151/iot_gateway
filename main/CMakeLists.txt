idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES app_provisioning app_sdcard app_rs485 app_rs232 app_pipeline app_mqtt app_rtc app_store_forward)
