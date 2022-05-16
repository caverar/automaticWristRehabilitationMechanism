/** 
  ******************************************************************************
  * @file    blinky_AO.c
  * @author  Camilo Vera
  * @brief   blinky active object
  *          This file constainst an implementation example of an active object
  *          using FreeAct over FreeRTOS 
  ****************************************************************************** 
*/

/* Includes ------------------------------------------------------------------*/
#include "blinky_AO.h"

// Standard C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


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

// Macros

#define TRIGGER_VOID_EVENT TimeEvent_arm(&this->te, (1 / portTICK_RATE_MS), 0U)

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
    
    // Time events construction
    TimeEvent_ctor(&this->te, BLINKY_AO_TIMEOUT_SIG, &this->super);
    
    // State Machine initialization
    this->state = BLINKY_AO_START_ST;

    // private data initialization
    this->isLedOn = false;
    this->number = 0;
    
    // Init code, preferably use bsp.c defined functions to control peripheral 
    // to keep encapsulation

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(TEST_PIN);
    gpio_set_dir(TEST_PIN, GPIO_OUT);
}

/* AO Class execution callback -----------------------------------------------*/
/**
 * @brief This function implements the code that will be executed concurrently 
 * with another AO, preferably using hierarchical state machines with event driven
 * paradigm in mind, which allows concurrency inside the AO itself.
 * 
 * @param this Object instance
 * @param e Events input
 */
static void BlinkyButton_dispatch(BlinkyButton * const this, 
                                  Event const * const e){
    // Initial event
    if(e->sig == INIT_SIG){
        TRIGGER_VOID_EVENT;
        // Initialization
        
        // Button press simulation to avoid connect physical button for test 
        // purposes, by triggering event
/*         // const Event button_pressed_event = {BLINKY_AO_BUTTON_PRESSED_SIG};
        // Active_post(AO_blinkyButton, (const Event*)&button_pressed_event); */
        
    }else{

    // State Machine 
    switch(this->state){
        case BLINKY_AO_START_ST:{
            switch (e->sig){
                case BLINKY_AO_TIMEOUT_SIG:{
                    this->state = BLINKY_AO_BLINKING_ST;
                    TimeEvent_arm(&this->te, (500 / portTICK_RATE_MS), 0U);
                    break;
                }default:
                    break;
            }
            break;

        }case BLINKY_AO_BLINKING_ST:{

            switch (e->sig){
                case BLINKY_AO_SW1_PRESSED_SIG:
                    printf("SW1_SIG\n");
                    break;

                case BLINKY_AO_SW2_PRESSED_SIG:
                    printf("SW2_SIG\n");
                    break;

                case BLINKY_AO_SW3_PRESSED_SIG:
                    printf("SW3_SIG\n");
                    break;

                case BLINKY_AO_SW4_PRESSED_SIG:
                    printf("SW4_SIG\n");
                    break;

                case BLINKY_AO_SW5_PRESSED_SIG:
                    printf("SW5_SIG\n");
                    break;

                case BLINKY_AO_TIMEOUT_SIG:{
                    if(!this->isLedOn){ /* LED not on */
                        gpio_put(LED_PIN, 1);
                        gpio_put(TEST_PIN, 1);
                        this->isLedOn = true;
                    }else{  /* LED is on */
                        gpio_put(LED_PIN, 0);
                        gpio_put(TEST_PIN, 0);
                        this->isLedOn = false;
                    }

                    TimeEvent_arm(&this->te, (300 / portTICK_RATE_MS), 0U);
                    static PRINTER_AO_TEXT_PL print_text1_event = 
                                                    {PRINTER_AO_TEXT0_SIG};

                    sprintf(print_text1_event.string_buffer,
                            "Numero: %03u        .",
                            this->number++);

                    Active_post(AO_printer, (Event*)&print_text1_event);                    
                    break;

                }default:
                    break;
            }
            break;

        }default:
            break;
    }
    }
}

/************************ Camilo Vera **************************END OF FILE****/