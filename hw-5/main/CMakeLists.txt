idf_component_register(SRCS "src/main.c" "src/feather_led.c" "src/switchdoc_labs.c" "src/esp_i2c.c" "src/buttons.c" 
                       PRIV_REQUIRES esp_driver_gpio freertos driver
                       INCLUDE_DIRS "include")
