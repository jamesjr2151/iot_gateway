idf_component_register(SRCS "app_provisioning.c"
                    INCLUDE_DIRS "include"
                    REQUIRES wifi_provisioning nvs_flash esp_wifi esp_netif esp_event log)
