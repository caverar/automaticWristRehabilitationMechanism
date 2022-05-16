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
#include "hardware/i2c.h"
//#include "hardware/uart.h"
//#include "hardware/gpio.h"

// FreeRTOS
#include <FreeRTOS.h>
#include <task.h>

// FreeAct
#include <FreeAct.h>

// Project libraries
#include "bsp.h"
#include "dev_hd44780.h"

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
    TimeEvent_ctor(&this->te, PRINTER_AO_TIMEOUT_SIG, &this->super);

    //stdio_init_all();

    i2c_init(i2c0, 100 * 1000);
    gpio_set_function(LCD_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_SDA_PIN);
    gpio_pull_up(LCD_SCL_PIN);

    dev_hd44780_init(i2c0, 0x27);

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
            // Clear screen
            dev_hd44780_text(i2c0, 0x27, 0, true, "                    ");
            dev_hd44780_text(i2c0, 0x27, 1, true, "                    ");
            dev_hd44780_text(i2c0, 0x27, 2, true, "                    ");
            dev_hd44780_text(i2c0, 0x27, 3, true, "                    ");
            break;

        case PRINTER_AO_TEXT0_SIG:{
            dev_hd44780_text(i2c0, 0x27, 0, true,
                             ((PRINTER_AO_TEXT_PL*)e)->string_buffer);
            break;

        case PRINTER_AO_TEXT1_SIG:{
            dev_hd44780_text(i2c0, 0x27, 1, true,
                             ((PRINTER_AO_TEXT_PL*)e)->string_buffer);
            break;
        }

        case PRINTER_AO_TEXT2_SIG:{
            dev_hd44780_text(i2c0, 0x27, 2, true,
                             ((PRINTER_AO_TEXT_PL*)e)->string_buffer);
            break;
        }

        case PRINTER_AO_TEXT3_SIG:{
            dev_hd44780_text(i2c0, 0x27, 3, true,
                             ((PRINTER_AO_TEXT_PL*)e)->string_buffer);
            break;
        }

        default: {
            break;
        }
    }
}

/************************ Camilo Vera **************************END OF FILE****/