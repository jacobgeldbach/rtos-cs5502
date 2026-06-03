/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 4
 * switches.c
 */
#include "common.h"

#include "seven_segment.h"
#include "buttons.h"
#include "counting.h"
#include "feather_led.h"

int shared_count = 0;
SemaphoreHandle_t count_mutex;
SemaphoreHandle_t count_enabled;
SemaphoreHandle_t display_mutex;

void app_main(void) {

    display_mutex = xSemaphoreCreateMutex();
    count_mutex = xSemaphoreCreateMutex();
    count_enabled = xSemaphoreCreateBinary();

    static int cc1 = 1;
    static int cc2 = 2;

    ESP_LOGI("Main", "Free Heap size %lu", esp_get_free_heap_size()); 

    xTaskCreatePinnedToCore(
        v_seven_segment_led,
        "Seven Segment first digit",
        2048,
        &cc1,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        v_seven_segment_led,
        "Seven Segment second digit",
        2048,
        &cc2,
        1,
        NULL,
        0
    );
    
    xTaskCreatePinnedToCore(
        v_button_detection,
        "Button detection",
        2048,
        NULL,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        v_blink_feather_led_task,
        "Feather LED blink and count",
        2048,
        NULL,
        1,
        NULL,
        0
    );
}
