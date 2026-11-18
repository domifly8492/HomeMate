idf_component_register(SRCS "svc_audio.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES app_events board drv_audio freertos
                       EMBED_FILES "sounds/chime.wav" "sounds/umbrella.wav")