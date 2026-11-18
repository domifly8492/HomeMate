idf_component_register(SRCS "ui_manager.c" "ui_status.c" "page_home.c" "ui_icons.c" "ui_popup.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES drv_display app_events svc_net svc_rules heap
                       EMBED_FILES "icons/weather_icons.bin")