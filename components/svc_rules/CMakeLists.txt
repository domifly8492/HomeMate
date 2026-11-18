idf_component_register(SRCS "svc_rules.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_events svc_audio svc_net nvs_flash esp_timer freertos)