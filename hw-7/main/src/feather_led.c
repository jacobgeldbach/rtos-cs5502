/*
 * CS 5502-71 Real Time Operating Systems
 * 
 * Author: Jacob A. Geldbach
 * Homework 4
 * feather_led.c
 */

#include "common.h"
#include "counting.h"

#include "driver/gpio.h"

void v_blink_feather_led_task(void* pv_parameters)
{

    /* Initial configuration for LED GPIO */
    gpio_reset_pin(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);

    while (1) {
        /* Set the LED GPIO pin high */
        gpio_set_level(GPIO_NUM_13, 1);
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Set the LED GPIO pin low */
        gpio_set_level(GPIO_NUM_13, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

}
