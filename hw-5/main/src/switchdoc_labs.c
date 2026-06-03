/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 5
 * switchdoc_labs.c
 *
 * Documentaion used: https://www.switchdoc.com/wp-content/uploads/2017/01/hdc1080.pdf
 */

#include "common.h"
#include "esp_i2c.h"

#define HDC1080_ADDR        0x40 /* 7 bit address b'1000000 */

#define TEMPERATURE_REG             0x00
#define HUMIDITY_REG                0x01
#define CONFIGURATION_REG           0x02
#define MANUFACTURER_ID_REG         0xFE
#define DEV_ID_REG                  0xFF

#define TEMP_HUMIDITY_FIELD         (1 << 4)

#define TEMPERATURE_READ_NOTIFICATION           1
#define HUMIDITY_READ_NOTIFICATION              2


static const char HDC1080_task_tag[40] = "HDC1080 i2c task";

/* Returns current temperature in degrees Celcius */
int read_and_calculate_temp(double *temp) {
    int ret = 0;
    uint8_t temperature_reg[2] = {0};
    uint8_t config_reg[2] = {0};
    
    /* Write config register for temperature only reading with smallest resolution */
    config_reg[0] = 0x6; /* In the MSByte we want bit 12 set to 0 for seperate temp/humidity reads
                            and bit 10 asserted for the smallest 11 bit resolution on the temperature reading
                            and bits 9:8 set to 0b'10 for smallest 8 bit resolution */
    if ((ret = i2c_write(HDC1080_ADDR, CONFIGURATION_REG, &config_reg[0], 2)) != 0) {
        ESP_LOGI(HDC1080_task_tag, "Error writing configuration register return: %d", ret);
        return -1;
    }
    
    /* Execute the temperature read, requires delay due to the reading trigger execution time */
    if ((ret = i2c_read_delay(HDC1080_ADDR, TEMPERATURE_REG, &temperature_reg[0], 2, 15)) != 0) {
        ESP_LOGI(HDC1080_task_tag, "Error reading temperature register return: %d", ret);
        return -1;
    }

    /* Get the 16 bit register value from the two bytes read over i2c */
    uint16_t temp_output = (temperature_reg[0] << 8) | temperature_reg[1];

    /* Actually calculate the temperature in degrees Celcius as per the HDC1080 specification document */
    *temp = ((double)temp_output / 65536.0) * 165.0 - 40.0;
    return 0;
}

/* Returns current humidity in percentage Realitive Humidity */
int read_and_calculate_humidity(double *humidity) {
    int ret = 0;
    uint8_t humidity_reg[2] = {0};
    uint8_t config_reg[2] = {0};
    
    /* Write config register for temperature/humidity only reading with smallest resolution */
    config_reg[0] = 0x6;
    if ((ret = i2c_write(HDC1080_ADDR, CONFIGURATION_REG, &config_reg[0], 2)) != 0) {
        ESP_LOGI(HDC1080_task_tag, "Error writing configuration register return: %d", ret);
        return -1;
    }

    /* Execute the humidity read, requires delay due to the reading trigger execution time */
    if ((ret = i2c_read_delay(HDC1080_ADDR, HUMIDITY_REG, &humidity_reg[0], 2, 15)) != 0) {
        ESP_LOGI(HDC1080_task_tag, "Error reading temperature register return: %d", ret);
        return -1;
    }

    /* Get the 16 bit register value from the two bytes read over i2c */
    uint16_t humidity_output = (humidity_reg[0] << 8) | humidity_reg[1];
    
    /* Actually calculate the percent relative humidity as per the HDC1080 specification document */
    *humidity = ((double)humidity_output / 65536.0) * 100.0;
    return 0;
}


void v_read_hdc1080_details_task(void* pv_parameters)
{
    ESP_LOGI(HDC1080_task_tag, "Starting task for HDC1080 i2c bus handling");

    QueueHandle_t signal_queue = (QueueHandle_t)pv_parameters;

    int ret;
    uint8_t hdc1080_manufacturer_id[2] = {0};
    uint8_t hdc1080_device_id[2] = {0};    
    /* Read the HDC1080 vendor and device IDs */
    if ((ret = i2c_read(HDC1080_ADDR, MANUFACTURER_ID_REG, &hdc1080_manufacturer_id[0], 2)) != 0) {
        ESP_LOGI(HDC1080_task_tag, "Error reading manufacturer id return: %d", ret);
    }   
    if ((ret = i2c_read(HDC1080_ADDR, DEV_ID_REG, &hdc1080_device_id[0], 2)) != 0) {
        ESP_LOGI(HDC1080_task_tag, "Error reading device id return: %d", ret);
    }

    ESP_LOGI(HDC1080_task_tag, "Manufacturer ID: 0x%02X%02X", hdc1080_manufacturer_id[0], hdc1080_manufacturer_id[1]);
    ESP_LOGI(HDC1080_task_tag, "Device ID: 0x%02X%02X", hdc1080_device_id[0], hdc1080_device_id[1]);

    while(1) {
        
        uint8_t notif = 0;
        if (xQueueReceive(signal_queue, &notif, portMAX_DELAY) == pdTRUE) {

            if (notif == TEMPERATURE_READ_NOTIFICATION) {
                double temp = 0;
                double temp_farenheit = 0;
                if (read_and_calculate_temp(&temp) != 0) {
                    ESP_LOGI(HDC1080_task_tag, "Error reading and calculating temperture");
                }

                temp_farenheit = (temp * 9.0 / 5.0) + 32.0;

                ESP_LOGI(HDC1080_task_tag, "Current temperature in degrees Celcius: %.1f", temp);
                ESP_LOGI(HDC1080_task_tag, "Current temperature in degrees Farenheit: %.1f", temp_farenheit);
            }

            if (notif == HUMIDITY_READ_NOTIFICATION) {
                double humidity = 0;

                if (read_and_calculate_humidity(&humidity) != 0) {
                    ESP_LOGI(HDC1080_task_tag, "Error reading and calculating humidity");
                }

                ESP_LOGI(HDC1080_task_tag, "Current percent relative humidity: %.1f", humidity);
            }
        }
    }
}
