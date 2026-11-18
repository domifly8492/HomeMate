idf_component_register(SRCS "drv_display.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES board esp_lcd driver freertos)