/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 4
 * switches.c
 */
#include "common.h"

#include "feather_led.h"
#include "stepper_motor.h"
#include "buttons.h"
#include "led.h"

QueueHandle_t led_cmd_queue = NULL;

void app_main(void) {
    static TaskHandle_t stepper_motor_handle = NULL;

    /* Queue to communicate stepper motor direction <> LED color/direction */
    led_cmd_queue = xQueueCreate(10, sizeof(led_cmd_t));
    
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
        drive_stepper_motor,
        "Stepper motor control",
        2048,
        NULL,
        1,
        &stepper_motor_handle,
        0
    );

    xTaskCreatePinnedToCore(
        v_light_led_strip,
        "led strip control",
        2048,
        NULL,
        1,
        NULL,
        0
    );

    /* Initialize the three button GPIOs as Interrupt service routines instead of the button polling task
     * With the stepper motor task as the target for the notification queue */
    btn1_ctx.notify_task = stepper_motor_handle;
    btn2_ctx.notify_task = stepper_motor_handle;
    btn3_ctx.notify_task = stepper_motor_handle;

    /* Only called once */
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    button_init(&btn1_ctx);
    button_init(&btn2_ctx);
    button_init(&btn3_ctx);
}
