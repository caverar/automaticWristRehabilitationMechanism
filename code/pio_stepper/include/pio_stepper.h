/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PIO_STEPPER_H
#define PIO_STEPPER_H



/* Includes ------------------------------------------------------------------*/

// Standard C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// SDK Libraries
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"



typedef struct{
    PIO pio;
    uint8_t sm;
    uint32_t dir_pin;
    uint32_t step_pin;
    uint32_t enable_pin;
    uint16_t steps_per_turn;
    uint16_t total_turns;
    uint32_t steps_frequency;
    uint32_t steps_pending;
    bool current_direction;

}StepperMotor;


void StepperMotor_ctor(StepperMotor* this,
                       PIO _pio,
                       uint8_t _sm,
                       uint32_t _dir_pin,
                       uint32_t _step_pin,
                       uint32_t _enable_pin,
                       uint16_t _steps_per_turn,
                       uint16_t _total_turns);


void StepperMotor_disable(StepperMotor* this);

void StepperMotor_enable(StepperMotor* this);



void StepperMotor_move(StepperMotor* this,
                       uint8_t dir, 
                       uint32_t _steps_frequency,
                       uint16_t _steps_pending);
#endif /* PIO_STEPPER_H */
/************************ Camilo Vera **************************END OF FILE****/
