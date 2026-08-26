/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Final
 * radar.c
 *
 */

/* This is named radar despite it being a ultrasonic (sonar) sensor because I wrote all the majority of all this code before I realized they were called radars due to the radio wave
 * as opposed to sound waves. I did not want to go back and find all the mentions of radar and change them */

#include "common.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "radar.h"

static const char hcsr04_task_tag[40] = "HCSR04 task";

#define TRIG_PIN         GPIO_NUM_22
#define ECHO_PIN         GPIO_NUM_23

radar_data_t latest_radar = {0};
SemaphoreHandle_t radar_mutex = NULL;

void v_hcsr04_task(void* pv_parameters) {

    ESP_LOGI(hcsr04_task_tag, "Starting task for Radar sensor");
    gpio_reset_pin(ECHO_PIN);
    gpio_reset_pin(TRIG_PIN);
    gpio_set_direction(ECHO_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(TRIG_PIN, GPIO_MODE_OUTPUT);
 
    gpio_set_level(TRIG_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    while(1) {
        
        /* Make sure it starts as 0 */
        gpio_set_level(TRIG_PIN, 0);
        esp_rom_delay_us(2);

        /* Send trigger */
        gpio_set_level(TRIG_PIN, 1);
        esp_rom_delay_us(10);
        gpio_set_level(TRIG_PIN, 0);
        uint64_t timeout = esp_timer_get_time() + 30000;

        /* Wait for ECHO to go low first */
        while(gpio_get_level(ECHO_PIN) == 1) {
            if (esp_timer_get_time() > timeout) {
                ESP_LOGI(hcsr04_task_tag, "Timeout waiting for ECHO LOW IDLE");
                goto next_reading;
            }
        }
        
        /* Wait for ECHO to go high for time measurement begin */
        while (gpio_get_level(ECHO_PIN) == 0) {
            if (esp_timer_get_time() > timeout) {
                ESP_LOGI(hcsr04_task_tag, "Timeout waiting for ECHO HIGH start measurement");
                goto next_reading;
            }
        }

        uint64_t start = esp_timer_get_time();
        
        /* wait for ECHO to go low end for end of measurement */
        while(gpio_get_level(ECHO_PIN) == 1) {
            if (esp_timer_get_time() > timeout) {
                ESP_LOGI(hcsr04_task_tag, "Timeout waiting for ECHO LOW end measurement");
                goto next_reading;
            }
        }

        uint64_t end = esp_timer_get_time();
        uint64_t pulse_us = end - start;

        float distance_inches = pulse_us / 148.0f;

        /* Take the radar struct mutex, copy the current_angle at time of distance measurement into target_angle
         * so that this target_distance corresponds with this exact moment current angle */
        xSemaphoreTake(radar_mutex, portMAX_DELAY);
        latest_radar.target_distance = distance_inches / 12.0f;
        latest_radar.in_range = (latest_radar.target_distance <= 6.0) ? true : false;
        xSemaphoreGive(radar_mutex);

    next_reading:
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
