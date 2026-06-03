/*
 * CS 5502-71 Real Time Operating Systems
 * 
 * Author: Jacob A. Geldbach
 * Homework 6
 * stepper_motor.c
 */

#include "common.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "buttons.h"

static const char stepper_motor_task_tag[40] = "Stepper Motor task";

#define HALFSTEPS_PER_ROTATION      4096
#define ROTATION_MASK               (HALFSTEPS_PER_ROTATION - 1)

#define MOTOR_PIN_A         GPIO_NUM_16
#define MOTOR_PIN_B         GPIO_NUM_17
#define MOTOR_PIN_C         GPIO_NUM_21
#define MOTOR_PIN_D         GPIO_NUM_14

#define PIN_A_MASK          (1 << 3)
#define PIN_B_MASK          (1 << 2)
#define PIN_C_MASK          (1 << 1)
#define PIN_D_MASK          (1 << 0)

uint8_t halfsteps[8] = { 0x8, 0xC, 0x4, 0x6, 0x2, 0x3, 0x1, 0x9 };
uint8_t fullsteps[4] = { 0x8, 0x4, 0x2, 0x1 };

void drive_stepper_motor(void* pv_parameters)
{
    ESP_LOGI(stepper_motor_task_tag, "Starting stepper motor driver task");
    ESP_LOGI(stepper_motor_task_tag, "Tick rate tick_hz=%d", configTICK_RATE_HZ);
    ESP_LOGI(stepper_motor_task_tag, "Ticks per MS with esp32 %d", pdMS_TO_TICKS(1));
    int count_half_steps = 0;

    /* Initial configuration for GPIO Pins connected to the stepper motor 4 control pins */
    gpio_reset_pin(MOTOR_PIN_A);
    gpio_reset_pin(MOTOR_PIN_B);
    gpio_reset_pin(MOTOR_PIN_C);
    gpio_reset_pin(MOTOR_PIN_D);
    gpio_set_direction(MOTOR_PIN_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_PIN_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_PIN_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(MOTOR_PIN_D, GPIO_MODE_OUTPUT);

    uint8_t run = 0;
    bool counter_clockwise = false;
    uint16_t us_delay[4] = { 0, 1200, 750, 500 };
    while (1) {
        uint32_t notif;
        if (xTaskNotifyWait(0, 0xFFFFFFFF, &notif, 0) > 0) {
            /* Using ISR so handle debounce here */
            vTaskDelay(pdMS_TO_TICKS(200));
            xTaskNotifyWait(0, 0xFFFFFFFF, NULL, 0);
            
            if (notif == BTN1) {
                run = 0;
            }
            else {

                run = (run + 1) % 4;

                if (notif == BTN2) {
                    if (counter_clockwise != false) {
                        run = 1;
                    }

                    counter_clockwise = false;
                }
                else if (notif == BTN3) {
                    if (counter_clockwise != true) {
                        run = 1;
                    }

                    counter_clockwise = true;
                }
                else {
                    ESP_LOGI(stepper_motor_task_tag, "Received unexpected button signal: %d", notif);
                }
            }

            ESP_LOGI(stepper_motor_task_tag, "Received button signal button_id: %d run_state %d", notif, run);

            if (!run) {
                gpio_set_level(MOTOR_PIN_A, 0);
                gpio_set_level(MOTOR_PIN_B, 0);
                gpio_set_level(MOTOR_PIN_C, 0);
                gpio_set_level(MOTOR_PIN_D, 0);
            }
        }
        
        if (run) {
            const int i = count_half_steps % 8;
            gpio_set_level(MOTOR_PIN_A, (halfsteps[i] & PIN_A_MASK) >> 3);
            gpio_set_level(MOTOR_PIN_B, (halfsteps[i] & PIN_B_MASK) >> 2);
            gpio_set_level(MOTOR_PIN_C, (halfsteps[i] & PIN_C_MASK) >> 1);
            gpio_set_level(MOTOR_PIN_D, (halfsteps[i] & PIN_D_MASK) >> 0);

            if (counter_clockwise == true) {
                count_half_steps = (count_half_steps + 1) & ROTATION_MASK;
            }
            else {
                count_half_steps = (count_half_steps - 1) & ROTATION_MASK;
            }
                    
            /* While running, need to use esp_rom_delay as even with 1000hz tick on esp vTaskDelay() minimum delay between steps is 1MS */
            esp_rom_delay_us(us_delay[run]);
        }
        else {
            uint32_t notif;
            xTaskNotifyWait(0, 0xFFFFFFFF, &notif, portMAX_DELAY);
            /* Using ISR so handle debounce here */
            vTaskDelay(pdMS_TO_TICKS(200));
            xTaskNotifyWait(0, 0xFFFFFFFF, NULL, 0); //Clear bounce notifications before moving on
            
            if (notif != BTN1) {
                run = (run + 1) % 4;

                if (notif == BTN2) {
                    if (counter_clockwise != false) {
                        run = 1;
                    }

                    counter_clockwise = false;
                }
                else if (notif == BTN3) {
                    if (counter_clockwise != true) {
                        run = 1;
                    }

                    counter_clockwise = true;
                }
                else {
                    ESP_LOGI(stepper_motor_task_tag, "Received unexpected button signal: %d", notif);
                }
            }
            else {
                /* BTN1 pressed twice in a row, return to initial position */
                while (count_half_steps != 0) {
                    const int i = count_half_steps % 8;
                    gpio_set_level(MOTOR_PIN_A, (halfsteps[i] & PIN_A_MASK) >> 3);
                    gpio_set_level(MOTOR_PIN_B, (halfsteps[i] & PIN_B_MASK) >> 2);
                    gpio_set_level(MOTOR_PIN_C, (halfsteps[i] & PIN_C_MASK) >> 1);
                    gpio_set_level(MOTOR_PIN_D, (halfsteps[i] & PIN_D_MASK) >> 0);

                    if (counter_clockwise == true) {
                        count_half_steps = (count_half_steps + 1) & ROTATION_MASK;
                    }
                    else {
                        count_half_steps = (count_half_steps - 1) & ROTATION_MASK;
                    }
                    
                    /* While running, need to use esp_rom_delay as even with 1000hz tick on esp vTaskDelay() minimum delay between steps is 1MS */
                    esp_rom_delay_us(us_delay[1]); //return to initial at slowest speed
                }
            }

            int state = (run) ? 1 : 0;
            ESP_LOGI(stepper_motor_task_tag, "Received button signal button_id: %d run_state %d", notif, state);
        }
    }
}


