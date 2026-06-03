/*
 * CS 5502-71 Real Time Operating Systems
 * 
 * Author: Jacob A. Geldbach
 * Homework 2
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "driver/gpio.h"

#define ESP_LED         GPIO_NUM_13

typedef enum {
    BTN1        = 0,
    BTN2,
    BTN3,
    NUM_BUTTONS
} switches_t;

static const gpio_num_t switches_esp_gpio_map[NUM_BUTTONS] = {
    [BTN1] =            GPIO_NUM_32,     //Valduino GP08
    [BTN2] =            GPIO_NUM_15,     //Valduino GP09
    [BTN3] =            GPIO_NUM_18,     //Valduino GP19
};


void app_main(void)
{ 
    /* Initial configuration for all the switch GPIO pins */
    int i;
    for (i = BTN1; i < NUM_BUTTONS; i++) {
        gpio_reset_pin(switches_esp_gpio_map[i]);
        gpio_set_direction(switches_esp_gpio_map[i], GPIO_MODE_INPUT);
    }

    /* Initial configuration for LED GPIO */
    gpio_reset_pin(ESP_LED);
    gpio_set_direction(ESP_LED, GPIO_MODE_OUTPUT);
    
    while (1) {
        
        /* Beginning of scan, assert ESP LED for 1s */
        gpio_set_level(ESP_LED, 1);
        usleep(1000000);
        gpio_set_level(ESP_LED, 0);

        for (i = BTN1; i < NUM_BUTTONS; i++) {
            int pin_level;
            pin_level = gpio_get_level(switches_esp_gpio_map[i]);

            printf("Button number: %d GPIO pin number %d pin: %d\n", i, switches_esp_gpio_map[i], pin_level);
            
            if (pin_level) {
                gpio_set_level(ESP_LED, 1);
            }

            usleep(500000);

            /* Before checking next switch, deassert LED GPIO */
            gpio_set_level(ESP_LED,0);
        }
    }

}
