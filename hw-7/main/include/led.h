/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 7
 * led_strip.h
 */

#include "common.h"

typedef struct {
    uint8_t         red;
    uint8_t         green;
    uint8_t         blue;
    uint8_t         white;
    uint16_t        speed; /* Delay between led being powered, lower is faster */
    bool            clockwise; /* counter_cw is false */
    bool            solid; /* informs if moving */
    bool            initial; /* informs if in initial position or not */
} led_cmd_t;

extern QueueHandle_t led_cmd_queue;

void v_light_led_strip(void* pv_parameters);
