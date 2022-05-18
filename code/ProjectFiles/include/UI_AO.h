#ifndef UI_AO_H
#define UI_AO_H

/** 
  ******************************************************************************
  * @file    UI_AO.h
  * @author  Alejandra Arias
  * @brief   ui active object
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

// Definitions
#define MAX_LENGTH_LCD 20

/* External AO calls --- -----------------------------------------------------*/
extern Active *AO_printer;
extern Active *AO_blinkyButton;
extern Active *AO_UI;

/* Constants definitions -----------------------------------------------------*/


/* AO Class input Signals ----------------------------------------------------*/

/**
 * @brief Here are define the input signals to this AO, default signals doesn't 
 * contain anything, but is possible to add payload by inheritance.
 */


enum UI_Signals { 
    UI_AO_TIMEOUT_SIG = USER_SIG,    // First Signals must replace USER_SIG
    // User Signals
    UI_AO_SW1_PRESSED_SIG,
    UI_AO_SW2_PRESSED_SIG,
    UI_AO_SW3_PRESSED_SIG,
    UI_AO_SW4_PRESSED_SIG,
    UI_AO_SW5_PRESSED_SIG
};


typedef struct{
    char title[20];
    uint num_of_options;
    char options[10][20];            
}UI_State;


typedef struct{
    uint type_of_exercise;
    uint num_of_reps;
    int8_t lim_min;
    uint lim_max;
    uint time_pos;          
}Exercise;

typedef struct{
    Exercise ejercicios[10];
    uint pause;        
}Routine;


    

/* AO Class Data -------------------------------------------------------------*/

typedef struct {
    Active super;               // Inherit from Active Object base class

    TimeEvent te;               // Add TimeEvent to the AO
    /* add private data (local variables) for the AO... */
    enum{
        UI_AO_HOME_ST,
        UI_AO_CALIBRATE_ST,
        UI_AO_INICIO_ST,
        UI_AO_CREATE_ST,
        UI_AO_DO_DEFAULT_ST,
        UI_AO_CONFIG_EXERCISE_ST,
        UI_AO_REPETITIONS_ST,
        UI_AO_MIN_ANGLE_ST,
        UI_AO_MAX_ANGLE_ST,
        UI_AO_TIME_ST,
        UI_AO_EXERCISE_READY_ST,
        UI_AO_BEGIN_ROUTINE_ST,
        UI_AO_CHECK_ROUTINE_ST,
        UI_AO_DO_ROUTINE_NOW_ST,
        UI_AO_SET_PAUSE_ST,
        UI_AO_SET_AS_DEFAULT_ST
    }state;

        // default values
    int8_t default_reps;
    int8_t default_min_angle;
    int8_t default_max_angle;
    int8_t default_time_in_position;
    int8_t default_pause_between_exercises;

    // exercise parameters

    int8_t reps;
    int8_t min_angle;
    int8_t max_angle;
    int8_t time_in_position;
    int8_t pause_between_exercises;

    // limit values for exercise parameters
    int8_t max_reps;
    int8_t max_secs;
    int8_t max_pause;

    // Value for exercise type
    u_int8_t exercise_type;

} UI;


/* AO Class execution callback -----------------------------------------------*/
/**
 * @brief This function implments the code that will be executed concurrently 
 * with anothe AO, preferably using herarchical state machines with event driven
 * paradigm in mind, which allows concurrency inside the AO itself.
 * 
 * @param this Object instance
 * @param e Events input
 */


static void UI_dispatch(UI * const this, 
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


void UI_ctor(UI * const this);



/* AO Class methods ----------------------------------------------------------*/

void UI_show_inicio(UI_State estado);
void UI_show_plus_button(UI_State estado);
void UI_show_minus_button(UI_State estado);
void UI_show(char* row1, char* row2, char* row3, char* row4);
void UI_selec(char* a);
#ifdef __cplusplus
}
#endif
#endif /* UI_AO_H */

/************************ Alejandra Arias **************************END OF FILE****/