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

#include "Motors_AO.h"

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
    TimeEvent_ctor(&this->te2, BLINKY_AO_TIMEOUT2_SIG, &this->super);
    
    // State Machine initialization
    this->state = BLINKY_AO_START_ST;

    // private data initialization
    this->isLedOn = false;
    this->number = 0;
    this->number2 = 0;
    
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

        static const Event start_motors_event = {MOTORS_AO_START_CALIB_SIG};
        Active_post(AO_Motors, &start_motors_event);


    }else if(e->sig == UI_AO_ACK_CALIB_SIG){

        TimeEvent_arm(&this->te, (2000 / portTICK_RATE_MS), 0U);
        

    }else if(e->sig == BLINKY_AO_TIMEOUT_SIG){
        static MOTORS_AO_MOVE_PL move_motor1_event = {MOTORS_AO_MOVE_SIG,
                                                            M1,-1200};
        Active_post(AO_Motors, (Event*)&move_motor1_event);

    }else if(e->sig == UI_AO_ACK_MOVE_SIG){
        static const Event free_motors_event = {MOTORS_AO_FREE_M1_SIG};
        Active_post(AO_Motors, (Event*)&free_motors_event);
        TimeEvent_arm(&this->te2, (1000 / portTICK_RATE_MS), 0U);

    }else if(e->sig == BLINKY_AO_TIMEOUT2_SIG){
        static const Event get_angle_event = {MOTORS_AO_RQ_DEG_M1_SIG};
        Active_post(AO_Motors, (Event*)&get_angle_event);
        TimeEvent_arm(&this->te2, (100 / portTICK_RATE_MS), 0U);

    }else if(e->sig == UI_AO_ACK_DEG_M1_SIG){
        printf("Angulo AO: %d\n",((UI_AO_ANGLE_PL*)e)->angle);
    }

}


/************************ Camilo Vera **************************END OF FILE****/