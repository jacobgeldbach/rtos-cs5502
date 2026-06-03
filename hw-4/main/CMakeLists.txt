idf_component_register(SRCS "main.c" "seven_segment.c" "buttons.c" "feather_led.c" 
                       PRIV_REQUIRES esp_driver_gpio freertos
                       INCLUDE_DIRS "include")
