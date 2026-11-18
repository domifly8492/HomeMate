idf_component_register(SRCS "drv_sensors.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES board app_events driver esp_timer freertos)