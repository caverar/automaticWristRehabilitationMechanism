#ifndef MOTORS_AO_H
#define MOTORS_AO_H

/** 
  ******************************************************************************
  * @file    Motors_AO.h
  * @author  Camilo Vera
  * @brief   Motors active object
  *          This file constains an implantation of stepper motors control
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
#include "hardware/pio.h"
//#include "hardware/gpio.h"

// FreeAct
#include <FreeAct.h>

// Project libraries
#include "pio_stepper.h"
#include "AS5600.h"

/* External AO calls --- -----------------------------------------------------*/

extern Active *AO_blinkyButton;

/* Constants definitions -----------------------------------------------------*/

#define MOTOR1_STEP_PIN 2
#define MOTOR1_DIR_PIN 3
#define MOTOR1_ENABLE_PIN 4

#define MOTOR2_STEP_PIN 5
#define MOTOR2_DIR_PIN 6
#define MOTOR2_ENABLE_PIN 7

#define ENCODER1_SDA_PIN 10
#define ENCODER1_SCL_PIN 11

#define ENCODER2_SDA_PIN 14
#define ENCODER2_SCL_PIN 15

#define END_SWITCH_1 8
#define END_SWITCH_2 9

#define MOTOR1_CALIB_FREQ 250
#define MOTOR1_CALIB_STEPS 1

#define MOTOR2_CALIB_FREQ 250
#define MOTOR2_CALIB_STEPS 1

#define MOTOR1_CENTER_FREQ 50
#define MOTOR1_CENTER_STEPS 2

#define MOTOR2_CENTER_FREQ 50
#define MOTOR2_CENTER_STEPS 2

#define MOTOR1_MOVEMENT_FREQ 50
#define MOTOR1_MOVEMENT_STEPS 2

#define MOTOR2_MOVEMENT_FREQ 50
#define MOTOR2_MOVEMENT_STEPS 2


#define MOTOR2_DEG_RANGE [-90, 90]
#define MOTOR2_DEG_RANGE_LEN 180
#define MOTOR2_TRANSMISSION_RATE 1
#define MOTOR2_STEPS_PER_REV 400
#define MOTOR2_FULL_RANGE_STEPS 200     // NUmber of steps in valid range
#define MOTOR2_HOME_TO_CENTER_STEPS 114
#define MOTOR2_DEG_PER_STEP (float)(360/(MOTOR2_TRANSMISSION_RATE*MOTOR2_STEPS_PER_REV))
#define MOTOR2_STEPS_PER_DEG (float)((MOTOR2_TRANSMISSION_RATE*MOTOR2_STEPS_PER_REV)/360)


#define MOTOR1_DEG_RANGE [-90, 110]     // End sensor at 110°
#define MOTOR1_DEG_RANGE_LEN 200
#define MOTOR1_TRANSMISSION_RATE 3
#define MOTOR1_STEPS_PER_REV 400
#define MOTOR1_FULL_RANGE_STEPS 600     // TODO Number of steps in valid range
#define MOTOR1_HOME_TO_CENTER_STEPS 290
#define MOTOR1_DEG_PER_STEP (float)(360/(MOTOR1_TRANSMISSION_RATE*MOTOR1_STEPS_PER_REV))
#define MOTOR1_STEPS_PER_DEG (float)((MOTOR1_TRANSMISSION_RATE*MOTOR1_STEPS_PER_REV)/360)


#define MOTOR1_POS_DIR  1    //(+)      // CCW positive right hand rule
#define MOTOR1_NEG_DIR  0    //(-)
#define ENCODER1_POS_DIR 1

#define MOTOR2_POS_DIR  0   //(+)       // CCW positive right hand rule
#define MOTOR2_NEG_DIR  1   //(-)
#define ENCODER2_POS_DIR 0

// Both encoders get negative numbers



/* AO Class input Signals ----------------------------------------------------*/

enum Motors_Signals{
    MOTORS_AO_TIMEOUT_SIG = USER_SIG,   // First Signals must replace USER_SIG
    // User Signals

    MOTORS_AO_START_CALIB_SIG,          // Calibration
    //-->UI_AO_ACK_CALIB_SIG

    MOTORS_AO_FREE_M1_SIG,               // Free motor 1
    MOTORS_AO_FREE_M2_SIG,               // Free motor 2
    
    MOTORS_AO_RQ_DEG_M1_SIG, 
    //-->UI_AO_ACK_DEG_M1_SIG

    MOTORS_AO_RQ_DEG_M2_SIG,
    //-->UI_AO_ACK_DEG_M2_SIG

    MOTORS_AO_BLOCK_M1_SIG,
    MOTORS_AO_BLOCK_M2_SIG,
    
    
    MOTORS_AO_MOVE_SIG,                 // Move
    //-->UI_AO_ACK_MOVE_SIG
    // Motor2
};

typedef struct{
    Event super;                        // Inherit from Event base class
    enum{M1, M2}motor;
    int16_t degrees;                    // en decimas de grado 
}MOTORS_AO_MOVE_PL;


typedef enum {
    MOTORS_AO_CALIB_M1_ST,
    MOTORS_AO_CALIB_M2_ST,

    MOTORS_AO_CENTER_M1_ST,
    MOTORS_AO_CENTER_M2_ST,
    MOTORS_AO_CENTER_M2_FREE_FIX_ST,

    MOTORS_AO_FREE_M1_ST,
    MOTORS_AO_FREE_M2_ST,

    MOTORS_AO_MOVE_M1_ST,
    MOTORS_AO_MOVE_M2_ST,

    MOTORS_AO_WAITING_ST
}Motors_AO_state;

typedef enum {
    CENTER_M1_PENDING_ST,
    CENTER_M1_DONE_ST
}Motors_AO_Center_M1_ST_state;

typedef enum {
    CENTER_M2_PENDING_ST,
    CENTER_M2_DONE_ST
    
}Motors_AO_Center_M2_ST_state;

/* AO Class Data -------------------------------------------------------------*/
typedef struct{
    Active super;                       // Inherit from Active Object base class
    TimeEvent te;                       // Add TimeEvent to the AO
    Motors_AO_state state;
    Motors_AO_state past_state;

    // Sub SM
    Motors_AO_Center_M1_ST_state center_m1_state; 
    Motors_AO_Center_M2_ST_state center_m2_state; 

    uint16_t encoder1_zero;
    uint16_t encoder2_zero;

    int32_t encoder1_current_angle;
    int32_t encoder2_current_angle;
    int16_t encoder1_turns;
    int16_t encoder2_turns;
    uint16_t encoder1_last_read;
    uint16_t encoder2_last_read;


    uint16_t centering_steps;           // Number of steps to do centering
    bool centering_dir;
    uint16_t movement_steps;            // Number of steps to do movement
    bool movement_dir;

    float motor1_current_position;
    float motor2_current_position;

    float motor1_goal_position;
    float motor2_goal_position;


    /* add private data (local variables) for the AO... */
    StepperMotor motor1;
    StepperMotor motor2;


}Motors;


static void Motors_dispatch(Motors * const this, 
                            Event const * const e);

void Motors_ctor(Motors * const this);

static uint16_t read_encoder1(void);
static uint16_t read_encoder2(void);


#ifdef __cplusplus
}
#endif
#endif /* MOTORS_AO_H */

/************************ Camilo Vera **************************END OF FILE****/