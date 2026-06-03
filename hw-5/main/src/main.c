/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 4
 * switches.c
 */
#include "common.h"

#include "feather_led.h"
#include "switchdoc_labs.h"
#include "buttons.h"

void app_main(void) {

    QueueHandle_t hdc1080_handle_queue = xQueueCreate(10, sizeof(uint8_t));

    xTaskCreatePinnedToCore(
        v_blink_feather_led_task,
        "Feather LED blink",
        2048,
        NULL,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        v_button_detection,
        "Button detection",
        2048,
        hdc1080_handle_queue,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        v_read_hdc1080_details_task,
        "Handle HDC1080 i2c",
        2048,
        hdc1080_handle_queue,
        1,
        NULL,
        0
    );
}
