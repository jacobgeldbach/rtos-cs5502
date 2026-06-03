/*
 * CS 5502-71 Real Time Operating Systems
 * 
 * Author: Jacob A. Geldbach
 * Homework 4 
 * buttons.c
 */

#include "common.h"
#include "counting.h"
#include "buttons.h"

button_isr_ctx_t btn1_ctx = {
    .notify_task = NULL,
    .button = BTN1
};

button_isr_ctx_t btn2_ctx = {
    .notify_task = NULL,
    .button = BTN2
};

button_isr_ctx_t btn3_ctx = {
    .notify_task = NULL,
    .button = BTN3
};

/* Task that essentially wakes often to check the button GPIOs for assert */
void v_button_detection(void* pv_parameters)
{
    static const char button_task_tag[40] = "Button detection task";

    QueueHandle_t signal_queue = (QueueHandle_t)pv_parameters;

    /* Initial configuration for all the button GPIO pins */
    int i;
    for (i = BTN1; i < NUM_BUTTONS; i++) {
        gpio_reset_pin(buttons_esp_gpio_map[i]);
        gpio_set_direction(buttons_esp_gpio_map[i], GPIO_MODE_INPUT);
    }

    ESP_LOGI(button_task_tag, "Starting task for button detection");
    while (1) {
        int pin_level = 0;
        if ((pin_level = (gpio_get_level(buttons_esp_gpio_map[BTN1]))) == 1) {

            /* Add some debounce/multiple press protections */
            pin_level = 0;
            vTaskDelay(pdMS_TO_TICKS(50));
            pin_level = (gpio_get_level(buttons_esp_gpio_map[BTN1]));
    
            if (pin_level) {
                uint8_t notif = 1;
                xQueueSend(signal_queue, &notif, 0);
            }

            /* Wait until button is clearly unpressed */
            while((pin_level = (gpio_get_level(buttons_esp_gpio_map[BTN1]))) == 1) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }

        }
       
        if ((pin_level = (gpio_get_level(buttons_esp_gpio_map[BTN2]))) == 1) {

            /* Add some debounce/multiple press protections */
            pin_level = 0;
            vTaskDelay(pdMS_TO_TICKS(50));
            pin_level = (gpio_get_level(buttons_esp_gpio_map[BTN2]));
    
            if (pin_level) {
                uint8_t notif = 2;
                xQueueSend(signal_queue, &notif, 0);
            }

            /* Wait until button is clearly unpressed */
            while((pin_level = (gpio_get_level(buttons_esp_gpio_map[BTN2]))) == 1) {
                vTaskDelay(pdMS_TO_TICKS(50));
            }

        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }

}

/* Actual service routine that gets called with the GPIO button interrupt */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    button_isr_ctx_t *ctx = (button_isr_ctx_t*)arg; 

    BaseType_t hp_task_woken = pdFALSE;

    /* Allows for us to send multiple values in the ISR instead of just a mutex counter give */
    xTaskNotifyFromISR(ctx->notify_task, ctx->button, eSetValueWithOverwrite, &hp_task_woken);

    if (hp_task_woken) portYIELD_FROM_ISR();
}

/* Initialize a GPIO button with a task notify ISR, needs task handler for notified task */
void button_init(button_isr_ctx_t *ctx)
{
    gpio_reset_pin(buttons_esp_gpio_map[ctx->button]);    
    gpio_set_direction(buttons_esp_gpio_map[ctx->button], GPIO_MODE_INPUT);
    
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << buttons_esp_gpio_map[ctx->button], //64 bit pin mask relative to the esp GPIO pin numbering I assume
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
    };
    gpio_config(&io);

    gpio_isr_handler_add(buttons_esp_gpio_map[ctx->button], button_isr_handler, (void *)ctx);

}
