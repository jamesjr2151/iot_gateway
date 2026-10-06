idf_component_register(SRCS "app_rs232.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver freertos app_pipeline)
