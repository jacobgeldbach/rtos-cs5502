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
    DIP1        = 0,
    DIP2,
    DIP3,
    DIP4,
    DIP5,
    DIP6,
    DIP7,
    DIP8,
    NUM_SWITCHES
} switches_t;

static const gpio_num_t switches_esp_gpio_map[NUM_SWITCHES] = {
    [DIP1] =            GPIO_NUM_21,     //Valduino GP06
    [DIP2] =            GPIO_NUM_14,     //Valduino GP07
    [DIP3] =            GPIO_NUM_22,     //Valduino SCL
    [DIP4] =            GPIO_NUM_23,     //Valduino SDA
    [DIP5] =            GPIO_NUM_13,     //Valduino GP13
    [DIP6] =            GPIO_NUM_26,     //Valduino GP26
    [DIP7] =            GPIO_NUM_17,     //Valduino GP00
    [DIP8] =            GPIO_NUM_16,     //Valduino GP01
};


void app_main(void)
{ 
    /* Initial configuration for all the switch GPIO pins */
    int i;
    for (i = DIP1; i < NUM_SWITCHES; i++) {
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

        for (i = DIP1; i < NUM_SWITCHES; i++) {
            int pin_level;
            if (i == DIP5) {
                /* DIP5 is GPIO13 the same as the ESP LED */
                gpio_set_direction(switches_esp_gpio_map[i], GPIO_MODE_INPUT);
                pin_level = gpio_get_level(switches_esp_gpio_map[i]);

                /* After we get the switches pin_level, switch it back to OUTPUT for the LED */
                gpio_set_direction(switches_esp_gpio_map[i], GPIO_MODE_OUTPUT);
            }
            else {
                pin_level = gpio_get_level(switches_esp_gpio_map[i]);
            }

            printf("Switch number: %d GPIO pin number %d pin: %d\n", i, switches_esp_gpio_map[i], pin_level);
            
            if (!pin_level) {
                gpio_set_level(ESP_LED, 1);
            }

            usleep(500000);

            /* Before checking next switch, deassert LED GPIO */
            gpio_set_level(ESP_LED,0);
        }
    }

}
