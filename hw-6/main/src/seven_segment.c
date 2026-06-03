/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 4
 * seven_segment.c
 */

#include "common.h"
#include "counting.h"

#include "driver/gpio.h"

/* defines for PCN (hardware mod) for HW-4 Seven Seg */
#define SSCC1         GPIO_NUM_27     //Valduino GP11
#define SSCC2         GPIO_NUM_33     //Valduino GP10
#define SSA           GPIO_NUM_26     //Valduino GP26
#define SSB           GPIO_NUM_25     //Valduino GP27
#define SSC           GPIO_NUM_19     //Valduino GP20    
#define SSD           GPIO_NUM_5      //Valduino GP18
#define SSE           GPIO_NUM_4      //Valduino GP25
#define SSF           GPIO_NUM_14     //Valduino GP07
#define SSG           GPIO_NUM_18     //Valduino GP19
#define SSDP          GPIO_NUM_16     //Valduino GP01

static const int segments[7] = {
    [0] =            SSA,
    [1] =            SSB,
    [2] =            SSC,
    [3] =            SSD,
    [4] =            SSE,
    [5] =            SSF,
    [6] =            SSG
};

void init_seven_segment(void) {
   
    gpio_reset_pin(SSCC1);
    gpio_set_direction(SSCC1, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SSCC2);    
    gpio_set_direction(SSCC2, GPIO_MODE_OUTPUT);
    gpio_reset_pin(SSDP);
    gpio_set_direction(SSDP, GPIO_MODE_OUTPUT);
                   
    int i;
    for (i = 0; i < 7; i++) {
        gpio_reset_pin(segments[i]);
        gpio_set_direction(segments[i], GPIO_MODE_OUTPUT);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void clear_seven_segment_digit(void) {

    int i;
    for (i = 0; i < 7; i++) {
        gpio_set_level(segments[i], 0);
    }
}

void light_seven_segment_digit(int count, int cc)
{
    int digit;

    /* Clear the digit driving gpio */
    /* Detemine what the digit should be depending on cc/task */
    if (cc == 1) {
        digit = (count / 10) % 10;
    }
    else {
        digit = count % 10;
    }

    /* Clear the segment lines */
    clear_seven_segment_digit();
    
    gpio_set_level(SSCC2, 1);
    gpio_set_level(SSCC1, 1);
  
    /* Set the segment lines depending on digit */ 
    switch (digit) {
        case 0:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSB, 1);
            gpio_set_level(SSC, 1);
            gpio_set_level(SSD, 1);
            gpio_set_level(SSE, 1);
            gpio_set_level(SSF, 1); 
            break;
        case 1:
            gpio_set_level(SSB, 1);
            gpio_set_level(SSC, 1);
            break;
        case 2:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSB, 1);
            gpio_set_level(SSG, 1);
            gpio_set_level(SSE, 1);
            gpio_set_level(SSD, 1);
            break;
        case 3:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSB, 1);
            gpio_set_level(SSG, 1);
            gpio_set_level(SSC, 1);
            gpio_set_level(SSD, 1);
            break;
        case 4:
            gpio_set_level(SSF, 1);
            gpio_set_level(SSG, 1);
            gpio_set_level(SSB, 1);
            gpio_set_level(SSC, 1);
            break;
        case 5:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSF, 1);
            gpio_set_level(SSG, 1);
            gpio_set_level(SSC, 1);
            gpio_set_level(SSD, 1);
            break;
        case 6:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSF, 1);
            gpio_set_level(SSG, 1);
            gpio_set_level(SSE, 1);
            gpio_set_level(SSC, 1);
            gpio_set_level(SSD, 1);
            break;
        case 7:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSB, 1);
            gpio_set_level(SSC, 1);
            break;
        case 8:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSB, 1);
            gpio_set_level(SSC, 1);
            gpio_set_level(SSD, 1);
            gpio_set_level(SSE, 1);
            gpio_set_level(SSF, 1);
            gpio_set_level(SSG, 1);
            break;
        case 9:
            gpio_set_level(SSA, 1);
            gpio_set_level(SSB, 1);
            gpio_set_level(SSF, 1);
            gpio_set_level(SSG, 1);
            gpio_set_level(SSC, 1);
            break;
        default:
            break;

    }

    /* Set the digit gpio depending on cc/task */
    if (cc == 1) {
        gpio_set_level(SSCC1, 0);
    }
    else {
        gpio_set_level(SSCC2, 0);
    }


}

void v_seven_segment_led(void* pv_parameters) 
{
    int cc = *(int *) pv_parameters;
    
    char ss_task_tag[20];
    sprintf(ss_task_tag, "7Segment_Digit_%d", cc);

    init_seven_segment();

    ESP_LOGI(ss_task_tag, "Starting task for seven segment digit %d", cc);
    while(1) {
        int current_count; 
        
        xSemaphoreTake(count_mutex, 2);
        current_count = shared_count;
        xSemaphoreGive(count_mutex);

        xSemaphoreTake(display_mutex, 2);
        light_seven_segment_digit(current_count, cc);
        xSemaphoreGive(display_mutex);

        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
}
