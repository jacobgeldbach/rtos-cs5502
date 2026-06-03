/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 4
 * seven_segment.h
 */
#include "driver/gpio.h"

typedef enum {
    BTN1        = 0,
    BTN2,
    BTN3,
    NUM_BUTTONS
} buttons_t;

static const gpio_num_t buttons_esp_gpio_map[NUM_BUTTONS] = {
    [BTN1] =            GPIO_NUM_32,     //Valduino GP08
    [BTN2] =            GPIO_NUM_15,     //Valduino GP09
    [BTN3] =            GPIO_NUM_18,     //Valduino GP19
};

typedef struct {
    TaskHandle_t notify_task;
    buttons_t button;
} button_isr_ctx_t;

extern button_isr_ctx_t btn1_ctx;
extern button_isr_ctx_t btn2_ctx;
extern button_isr_ctx_t btn3_ctx;

void v_button_detection(void* pv_parameters);
void button_init(button_isr_ctx_t *ctx);
