/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 5
 * i2c.c
 */
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 23
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_NUM I2C_NUM_0

int i2c_master_initialized = 0;

void init_i2c_esp_master(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };

    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    i2c_master_initialized = 1;
}

int i2c_write(uint8_t device_addr, uint8_t reg, uint8_t *data, uint8_t data_len)
{
    if (!i2c_master_initialized) {
        init_i2c_esp_master();
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd); //Start building i2c byte stream/command
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true); //Shift the R/W bit low onto the addr byte after the 7 addr bits
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, data_len, true);
    i2c_master_stop(cmd); //Terminate command

    int ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, (1000 / portTICK_PERIOD_MS)); //Send the actual command over the i2c bus
    i2c_cmd_link_delete(cmd); 
    return ret;
}

int i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *data, uint8_t data_len) 
{
    if (!i2c_master_initialized) {
        init_i2c_esp_master();
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    /* Write register address into the Pointer Register with the R/W bit low */
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);

    /* Restart condition and write addr byte with device_addr and R/W bit high to initiate read command */
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    
    /* Build a command with ACK after each byte until the last byte */
    if (data_len > 1) {
        i2c_master_read(cmd, data, data_len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + data_len - 1, I2C_MASTER_NACK); //Build last read byte cmd in the chain with a NACK to terminate the read
    
    i2c_master_stop(cmd);

    int ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, (1000 / portTICK_PERIOD_MS)); //Send the command chain over the i2c bus
    i2c_cmd_link_delete(cmd);

    return ret;
}

/* Some devices take time for measurements after the read transaction begins */
int i2c_read_delay(uint8_t device_addr, uint8_t reg, uint8_t *data, uint8_t data_len, uint8_t delay_ms) 
{
    if (!i2c_master_initialized) {
        init_i2c_esp_master();
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    /* Write register address into the Pointer Register with the R/W bit low */
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);
    
    /* Send the device address and write bit + register pointer cmd over the i2c bus before the delay */
    int ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(1000)); //Send the command chain over the i2c bus 
    i2c_cmd_link_delete(cmd);
    
    vTaskDelay(pdMS_TO_TICKS(delay_ms));

    i2c_cmd_handle_t cmd1 = i2c_cmd_link_create();
    
    /* Restart condition and write addr byte with device_addr and R/W bit high to initiate read command */
    i2c_master_start(cmd1);
    i2c_master_write_byte(cmd1, (device_addr << 1) | I2C_MASTER_READ, true);
    
    /* Build a command with ACK after each byte until the last byte */
    if (data_len > 1) {
        i2c_master_read(cmd1, data, data_len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd1, data + data_len - 1, I2C_MASTER_NACK); //Build last read byte cmd in the chain with a NACK to terminate the read
    i2c_master_stop(cmd1);

    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd1, pdMS_TO_TICKS(1000)); //Send the command chain over the i2c bus
    i2c_cmd_link_delete(cmd1);

    return ret;
}
