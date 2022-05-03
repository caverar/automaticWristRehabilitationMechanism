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


void BSP_init(void);
// void BSP_start(void);
// void BSP_led0_on(void);
// void BSP_led0_off(void);
// void BSP_led1_on(void);
// void BSP_led1_off(void);

// enum Signals {
//     TIMEOUT_SIG = USER_SIG,
//     BUTTON_PRESSED_SIG,
//     BUTTON_RELEASED_SIG,
// };

//extern Active *AO_blinkyButton;

#endif /* BSP_H */