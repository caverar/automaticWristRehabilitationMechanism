#ifndef BLINKY_AO_H
#define BLINKY_AO_H

/** 
  ******************************************************************************
  * @file    blinky_AO.h
  * @author  Camilo Vera
  * @brief   blinky active object
  *          This file constainst an implentation example of an active object
  *          using FreeAct over FreeRTOS 
  ****************************************************************************** 
*/

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
//#include "hardware/uart.h"
//#include "hardware/gpio.h"

// FreeAct
#include <FreeAct.h>

/* External AO calls --- -----------------------------------------------------*/
extern Active *AO_printer;


/* Constants definitions -----------------------------------------------------*/
// const uint LED_PIN = PICO_DEFAULT_LED_PIN;
// const uint TEST_PIN = 15;

/* AO Class input Signals ----------------------------------------------------*/

/**
 * @brief Here are define the input signals to this AO, default signals doesn't 
 * contain anything, but is possible to add payload by inheritance.
 */

enum Signals {
    TIMEOUT_SIG = USER_SIG,     // First Signal always must replace USER_SIG
    BUTTON_PRESSED_SIG,
    BUTTON_RELEASED_SIG,
};


/* AO Class Data -------------------------------------------------------------*/
typedef struct {
    Active super;               // Inherit from Active Object base class
    TimeEvent te;               // Add TimeEvent to the AO
    /* add private data (local variables) for the AO... */

    bool isLedOn;               // Led State
} BlinkyButton;


/* AO Class execution callback -----------------------------------------------*/
/**
 * @brief This function implments the code that will be executed concurrently 
 * with anothe AO, preferably using herarchical state machines with event driven
 * paradigm in mind, which allows concurrency inside the AO itself.
 * 
 * @param this Object instance
 * @param e Events input
 */
static void BlinkyButton_dispatch(BlinkyButton * const this, 
                                  Event const * const e);


/* AO Class Constructor ------------------------------------------------------*/
/**
 * @brief This function implements the initialization of the AO, and is the 
 * proper place to execute peripheral initialization, initial states definition 
 * and assignation of variable initial values, as long as user input isn't 
 * required.
 *
 * @param this Object instance
 */
void BlinkyButton_ctor(BlinkyButton * const this);



/* AO Class methods ----------------------------------------------------------*/



#ifdef __cplusplus
}
#endif
#endif /* BLINKY_AO_H */

/************************ Camilo Vera **************************END OF FILE****/