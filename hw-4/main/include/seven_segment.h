/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 4
 * seven_segment.h
 */

#include <stdio.h>
#include <unistd.h>
#include "freertos/semphr.h"

extern SemaphoreHandle_t display_mutex;

void v_seven_segment_led(void* pv_parameters);
