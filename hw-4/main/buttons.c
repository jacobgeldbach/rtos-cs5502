/*
 * CS 5502-71 Real Time Operating Systems
 * 
 * Author: Jacob A. Geldbach
 * Homework 4 
 * buttons.c
 */

#include "common.h"
#include "counting.h"

#include "driver/gpio.h"
#include "esp_task_wdt.h"

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

/* This will keep track of the counting state */    
static bool counting = false;

void v_button_detection(void* pv_parameters)
{
    static const char button_task_tag[40] = "Button detection task";

    /* Initial configuration for all the button GPIO pins */
    int i;
    for (i = BTN1; i < NUM_BUTTONS; i++) {
        gpio_reset_pin(switches_esp_gpio_map[i]);
        gpio_set_direction(switches_esp_gpio_map[i], GPIO_MODE_INPUT);
    }

    ESP_LOGI(button_task_tag, "Starting task for button detection");
    while (1) {
        int pin_level = 0;
        if ((pin_level = (gpio_get_level(switches_esp_gpio_map[BTN1]))) == 1) {

            /* Add some debounce/multiple press protections */
            pin_level = 0;
            vTaskDelay(pdMS_TO_TICKS(50));
            pin_level = (gpio_get_level(switches_esp_gpio_map[BTN1]));
    
            if (pin_level) {
                /* Switch to the other counting state */
                counting = !counting;

                if (counting) {
                    xSemaphoreGive(count_enabled);
                    ESP_LOGI(button_task_tag, "Enabling Seven Segment Counting");
                }
                else {
                    xSemaphoreTake(count_enabled, portMAX_DELAY);
                    ESP_LOGI(button_task_tag, "Disabling Seven Segment Counting");
                }
            }

            /* Wait until button is clearly unpressed */
            while((pin_level = (gpio_get_level(switches_esp_gpio_map[BTN1]))) == 1) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }

        }
        
        vTaskDelay(pdMS_TO_TICKS(20));
    }

}
