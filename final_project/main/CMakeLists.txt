idf_component_register(SRCS "src/main.c"  "src/buttons.c" "src/stepper_motor.c" "src/led.c" "src/server.c" "src/radar.c"
                       PRIV_REQUIRES esp_driver_gpio freertos esp_wifi esp_event esp_netif nvs_flash esp_timer 
                       INCLUDE_DIRS "include")
