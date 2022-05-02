/** 
  ******************************************************************************
  * @file    printer_AO.c
  * @author  Camilo Vera
  * @brief   printer active object
  *          This file constainst an implentation example of an active object
  *          using FreeAct over FreeRTOS 
  ****************************************************************************** 
*/

/* Includes ------------------------------------------------------------------*/
#include "printer_AO.h"

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
void Printer_ctor(Printer * const this){
    Active_ctor(&this->super, (DispatchHandler)&Printer_dispatch);
    TimeEvent_ctor(&this->te, PRINTER_TIMEOUT_SIG, &this->super);

    stdio_init_all();
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
static void Printer_dispatch(Printer * const this, 
                                  Event const * const e){
    switch (e->sig) {
        case INIT_SIG:      // This event is always excuted at the beginning.
        case PRINTER_TIMEOUT_SIG: { // Time trigger event

            printf("Hola mundo\n");
            TimeEvent_arm(&this->te, (500 / portTICK_RATE_MS), 0U);
            break;
        }
        case TEXT: {
            printf(((const PRINTER_EVENT_TEXT*)e)->string_buffer);
        }

        default: {
            break;
        }
    }
}

/************************ Camilo Vera **************************END OF FILE****/