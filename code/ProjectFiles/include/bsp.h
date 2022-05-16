#ifndef BSP_H
#define BSP_H

// Standard C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// SDK Libraries
#include "pico/stdlib.h"
//#include "hardware/uart.h"
//#include "hardware/gpio.h"

// FreeAct
#include <FreeAct.h>
// External AO calls
extern Active *AO_printer;
extern Active *AO_blinkyButton;

// Global defines

#define LED_PIN PICO_DEFAULT_LED_PIN
#define TEST_PIN 15
#define BUTTON_PIN 14


#define SW1_MIN_VAL 0
#define SW1_MAX_VAL 50
#define SW2_MIN_VAL 100
#define SW2_MAX_VAL 200
#define SW3_MIN_VAL 300
#define SW3_MAX_VAL 450
#define SW4_MIN_VAL 600
#define SW4_MAX_VAL 750
#define SW5_MIN_VAL 1400
#define SW5_MAX_VAL 1500



#define DEBOUNCE_HIGH_LEVEL 101

uint8_t buttons_past_states;
uint8_t buttons_states;
uint16_t buttons_levels[5];
// uint8_t buttons_past_states = 0x00;
// uint8_t buttons_states = 0x00;
// uint16_t buttons_levels[5] = {0,0,0,0,0};

bool sw_debounce(bool current_state, u_int16_t* level);
void debounce_all(uint16_t adc_val,
                  uint16_t* buttons_levels,
                  uint8_t* buttons_states);

void BSP_init(void);
// void BSP_start(void);


extern Active *AO_blinkyButton;

#endif /* BSP_H */