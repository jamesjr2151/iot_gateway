idf_component_register(SRCS "app_sdcard.c"
                       INCLUDE_DIRS "include"
                       REQUIRES fatfs sdmmc driver)
