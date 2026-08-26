/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Final
 * server.h
 */

#include "common.h"

typedef struct {
    float       current_angle;
    float       target_angle;
    float       target_distance;
    bool        in_range;
} radar_data_t;

/* Struct for data from radar task and stepper motor task */
extern radar_data_t latest_radar;
/* Mutex to protect the shared struct between server/radar/stepper tasks */
extern SemaphoreHandle_t radar_mutex;

void v_hcsr04_task(void* pv_parameters);
