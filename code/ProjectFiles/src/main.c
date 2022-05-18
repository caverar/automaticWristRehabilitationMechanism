/** 
  ******************************************************************************
  * @file    main.c
  * @author  Camilo Vera
  * @brief   Main program.
  *          Description of the file functions, etc. 
  *
  @verbatim
  ==============================================================================
                     ##### Project Description #####
  ==============================================================================
  Example 02: FreeAct.
 
  This code is a test with FreeACT running on RP2040.
   
  MCU: RP2040 (Two Cortex M0+).
  @endverbatim
  ****************************************************************************** 
*/

/* Includes ------------------------------------------------------------------*/

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

// Active Objects
#include "blinky_AO.h"
#include "printer_AO.h"
#include "UI_AO.h"

// Project libraries
#include "bsp.h"


// Task Data
static StackType_t blinkyButton_stack[configMINIMAL_STACK_SIZE]; // task stack
static Event *blinkyButton_queue[10];
static BlinkyButton blinkyButton;

static StackType_t printer_stack[configMINIMAL_STACK_SIZE]; // task stack
static Event *printer_queue[10];
static Printer printer;

static StackType_t UI_stack[configMINIMAL_STACK_SIZE]; // task stack
static Event *UI_queue[10];
static UI ui;

//object static instance and inheritance from Active class:
Active *AO_blinkyButton = &blinkyButton.super;
Active *AO_printer = &printer.super;
Active *AO_UI = &ui.super;



int main(){
    
    stdio_init_all();
    BSP_init();

    /* create and start the BlinkyButton AO */
    BlinkyButton_ctor(&blinkyButton);
    Active_start(AO_blinkyButton,
                 1U,
                 blinkyButton_queue,
                 sizeof(blinkyButton_queue)/sizeof(blinkyButton_queue[0]),
                 blinkyButton_stack,
                 sizeof(blinkyButton_stack),
                 0U);

    Printer_ctor(&printer);
    Active_start(AO_printer,
                 1U,
                 (Event**)printer_queue,
                 sizeof(printer_queue)/sizeof(printer_queue[0]),
                 printer_stack,
                 sizeof(printer_stack),
                 0U);

    UI_ctor(&ui);
    Active_start(AO_UI,
                 1U,
                 UI_queue,
                 sizeof(UI_queue)/sizeof(UI_queue[0]),
                 UI_stack,
                 sizeof(UI_stack),
                 0U);


    //BSP_start(); /* configure and start interrupts */
    vTaskStartScheduler(); /* start the FreeRTOS scheduler... */
    return 0; /* NOTE: the scheduler does NOT return */

}