/** 
  ******************************************************************************
  * @file    blinky_AO.c
  * @author  Camilo Vera
  * @brief   blinky active object
  *          This file constainst an implentation example of an active object
  *          using FreeAct over FreeRTOS 
  ****************************************************************************** 
*/

/* Includes ------------------------------------------------------------------*/
#include "blinky_AO.h"

// Standard C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


// SDK Libraries
#include "pico/stdlib.h"
//#include "hardware/uart.h"
//#include "hardware/gpio.h"

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// FreeAct
#include <FreeAct.h>

// Another AO

#include "printer_AO.h"

// Project libraries
#include "bsp.h"

/* Implementation ------------------------------------------------------------*/



/* AO Class Constructor ------------------------------------------------------*/
/**
 * @brief This function implements the initialization of the AO, and is the 
 * proper place to execute peripheral initialization, initial states definition 
 * and assignation of variable initial values, as long as user input isn't 
 * required.
 *
 * @param this Object instance
 */
void BlinkyButton_ctor(BlinkyButton * const this){
    Active_ctor(&this->super, (DispatchHandler)&BlinkyButton_dispatch);
    TimeEvent_ctor(&this->te, TIMEOUT_SIG, &this->super);
    
    // private data initialization
    this->isLedOn = false;

    // Init code, preferably use bsp.c defined functions to control peripheral 
    // to keep encapsulation

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(TEST_PIN);
    gpio_set_dir(TEST_PIN, GPIO_OUT);
}

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
                                  Event const * const e){
    switch (e->sig) {
        case INIT_SIG:      // This event is always excuted at the beginning.
        case TIMEOUT_SIG: { // Time trigger event
            //static PRINTER_EVENT_TEXT buttonPressedEvt;
            

            static PRINTER_EVENT_TEXT const print_text_event = 
                        {TEXT, "Hola desde el otro mundo\n"};

            Active_post(AO_printer, (const Event*)&print_text_event);

            if (!this->isLedOn) { /* LED not on */
                gpio_put(LED_PIN, 1);
                gpio_put(TEST_PIN, 1);
                this->isLedOn = true;
                TimeEvent_arm(&this->te, (500 / portTICK_RATE_MS), 0U);
            }
            else {  /* LED is on */
                gpio_put(LED_PIN, 0);
                gpio_put(TEST_PIN, 0);
                this->isLedOn = false;
                TimeEvent_arm(&this->te, (500 / portTICK_RATE_MS), 0U);
            }
            break;
        }
        // case BUTTON_PRESSED_SIG: {
        //     BSP_led1_on();
        //     break;
        // }
        // case BUTTON_RELEASED_SIG: {
        //     BSP_led1_off();
        //     break;
        // }
        default: {
            break;
        }
    }
}



/************************ Camilo Vera **************************END OF FILE****/