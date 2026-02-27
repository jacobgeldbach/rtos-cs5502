idf_component_register(SRCS "blink_led.c"
                       PRIV_REQUIRES esp_driver_gpio freertos
                       INCLUDE_DIRS "")
