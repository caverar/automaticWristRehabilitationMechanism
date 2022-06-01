/** 
  ******************************************************************************
  * @file    AS5600.h
  * @author  Camilo Vera
  * @brief   AS5600 encoder library
  *          Description of the file functions, etc. 
  ****************************************************************************** 
*/

#ifndef AS5600_H
#define AS5600_H

#ifdef __cplusplus
extern "C" {
#endif


/* Includes ------------------------------------------------------------------*/

// Standard C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// SDK Libraries
#include "pico/stdlib.h"
#include "hardware/i2c.h"


#define device_address 0x36
#define raw_angle_address 0x0E

static void read_register(i2c_inst_t* i2c_instance, 
                   uint8_t device_addr,
                   uint8_t register_address,
                   uint8_t* read_buffer, 
                   uint8_t size);

uint16_t AS5600_read_angle(i2c_inst_t* i2c);


void AS5600_i2c_init(i2c_inst_t* i2c);
void AS5600_config_pull_up(uint32_t sda_pin, uint32_t scl_pin);
void AS5600_config_pins(uint32_t sda_pin, uint32_t scl_pin);
void AS5600_free_pins(uint32_t sda_pin, uint32_t scl_pin);

#ifdef __cplusplus
}
#endif

#endif // _DEV_HD44780_H_