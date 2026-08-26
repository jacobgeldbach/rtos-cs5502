/*
 * CS 5502-71 Real Time Operating Systems
 * 
 * Author: Jacob A. Geldbach
 * Homework 2
 *
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{

    /* Initial configuration for LED GPIO */
    gpio_reset_pin(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_OUTPUT);

    while (1) {
        int i;
        for (i = 0; i < 5; i++) {
            /* Set the LED GPIO pin high */
            gpio_set_level(GPIO_NUM_13, 1);
            vTaskDelay(pdMS_TO_TICKS(500));

            /* Set the LED GPIO pin low */
            gpio_set_level(GPIO_NUM_13, 0);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        /* Full cycle complete, remain off for 5 seconds before repeating */
        vTaskDelay(pdMS_TO_TICKS(5000));
    }

}
