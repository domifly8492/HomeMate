idf_component_register(SRCS "svc_net.c" "svc_weather.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_events esp_wifi esp_event esp_netif nvs_flash lwip
                                     freertos esp_http_client esp-tls json mbedtls)