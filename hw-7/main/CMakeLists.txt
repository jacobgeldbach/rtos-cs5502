idf_component_register(SRCS "src/main.c" "src/feather_led.c" "src/buttons.c" "src/stepper_motor.c" "src/led.c"
                       PRIV_REQUIRES esp_driver_gpio freertos 
                       INCLUDE_DIRS "include")
