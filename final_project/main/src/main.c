/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Final
 * main.c
 */
#include "common.h"

#include "stepper_motor.h"
#include "buttons.h"
#include "server.h"
#include "radar.h"

QueueHandle_t led_cmd_queue = NULL;

void app_main(void) {
    static TaskHandle_t stepper_motor_handle = NULL;

    /* Protects latest_radar shared struct */
    radar_mutex = xSemaphoreCreateMutex();
    stream_radar_init();
   
    configASSERT(radar_mutex != NULL);

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
        v_hcsr04_task,
        "Ultra Sonic sensor task for HCSR04",
        8192,
        NULL,
        1,
        NULL,
        0
    );

    xTaskCreatePinnedToCore(
        v_web_server,
        "web server handling task",
        8192,
        NULL,
        1,
        NULL,
        1
    );

    xTaskCreatePinnedToCore(
        v_stream_radar_data_task,
        "web server handling task",
        8192,
        NULL,
        1,
        NULL,
        1
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
