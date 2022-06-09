/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pio_stepper.h"
#include "stepper.pio.h"

/* Includes ------------------------------------------------------------------*/

// Standard C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// SDK Libraries
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#define MOTOR1_STEP_PIN 2
#define MOTOR1_DIR_PIN 3
#define MOTOR1_ENABLE_PIN 4

void StepperMotor_ctor(StepperMotor* this,
                       PIO _pio,
                       uint8_t _sm,
                       uint32_t _dir_pin,
                       uint32_t _step_pin,
                       uint32_t _enable_pin,
                       uint16_t _steps_per_turn,
                       uint16_t _total_turns){

    this->pio = _pio;
    this->sm = _sm;
    this->dir_pin = _dir_pin;
    this->step_pin = _step_pin;
    this->enable_pin = _enable_pin;
    this->steps_per_turn = _steps_per_turn;
    this->total_turns = _total_turns;

    this->current_direction = 0;
    this->steps_frequency = 15;
    this->steps_pending = 0;

    
    gpio_init(this->dir_pin);
    gpio_init(this->enable_pin);

    //gpio_set_dir(MOTOR1_STEP_PIN, GPIO_OUT);
    gpio_set_dir(this->dir_pin, GPIO_OUT);
    gpio_set_dir(this->enable_pin, GPIO_OUT);

    gpio_put(this->dir_pin, true);
    gpio_put(this->enable_pin, false);

    uint offset = pio_add_program(this->pio, &stepper_program);
    float div = (float)clock_get_hz(clk_sys) / (this->steps_frequency*4); 
    pio_stepper_init(this->pio, this->sm,offset,this->step_pin, div);
    pio_sm_set_enabled(this->pio, this->sm, true);

}


void StepperMotor_disable(StepperMotor* this){
    gpio_put(this->enable_pin, true);
}

void StepperMotor_enable(StepperMotor* this){
    gpio_put(this->enable_pin, false);
}



void StepperMotor_move(StepperMotor* this,
                       uint8_t dir, 
                       uint32_t _steps_frequency,
                       uint16_t _steps_pending){


    this->steps_frequency = _steps_frequency;
    this->steps_pending = _steps_pending;
    this->current_direction = dir;

    gpio_put(this->dir_pin, dir);
    float div = (float)clock_get_hz(clk_sys) / (this->steps_frequency*4);
    pio_sm_set_clkdiv(this->pio,this->sm,div);
    
    StepperMotor_enable(this);
    pio_sm_put(this->pio, this->sm, this->steps_pending-1);
}

