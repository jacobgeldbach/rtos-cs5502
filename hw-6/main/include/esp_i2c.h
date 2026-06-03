/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Homework 5
 * i2c.h
 */

#include "common.h"

int i2c_write(uint8_t device_addr, uint8_t reg, uint8_t *data, uint8_t data_len);
int i2c_read(uint8_t device_addr, uint8_t reg, uint8_t *data, uint8_t data_len);
int i2c_read_delay(uint8_t device_addr, uint8_t reg, uint8_t *data, uint8_t data_len, uint8_t delay_ms); 
