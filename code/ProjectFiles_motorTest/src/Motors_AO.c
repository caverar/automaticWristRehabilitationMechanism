
/** 
  ******************************************************************************
  * @file    Motors_AO.c
  * @author  Camilo Vera
  * @brief   Motors active object
  *          This file constainst an implantation of stepper motors control
  *          using FreeAct over FreeRTOS 
  ****************************************************************************** 
*/

/* Includes ------------------------------------------------------------------*/
#include "Motors_AO.h"
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
#include "blinky_AO.h"

// Project libraries
#include "pio_stepper.h"
#include "AS5600.h"


#define TRIGGER_VOID_EVENT TimeEvent_arm(&this->te, (1 / portTICK_RATE_MS), 0U)

void Motors_ctor(Motors * const this){
    Active_ctor(&this->super, (DispatchHandler)&Motors_dispatch);
    
    // Time events construction
    TimeEvent_ctor(&this->te, MOTORS_AO_TIMEOUT_SIG, &this->super);
    
    // State Machine initialization
    this->state = MOTORS_AO_CALIB_M1_ST;
    this->past_state = MOTORS_AO_CALIB_M1_ST;

    this->center_m1_state = CENTER_M1_PENDING_ST; 
    this->center_m2_state = CENTER_M2_PENDING_ST; 
    // private data initialization
    
    this->centering_steps = 0;
    this->movement_steps = 0;
    this->motor1_current_position = 0;
    this->motor2_current_position = 0;
    this->motor1_goal_position = 0;
    this->motor2_goal_position = 0;
    this->movement_dir = false;

    this->encoder1_current_angle = 0;
    this->encoder2_current_angle = 0;
    this->encoder1_turns = 0;
    this->encoder2_turns = 0;
    this->encoder1_last_read = 0;
    this->encoder2_last_read = 0;


    // Init code, preferably use bsp.c defined functions to control peripheral 
    // to keep encapsulation
    StepperMotor_ctor(&(this->motor1), pio0, 0, MOTOR1_DIR_PIN, 
                  MOTOR1_STEP_PIN, MOTOR1_ENABLE_PIN, 400, 3);
    StepperMotor_ctor(&(this->motor2), pio1, 0, MOTOR2_DIR_PIN, 
                  MOTOR2_STEP_PIN, MOTOR2_ENABLE_PIN, 400, 1);
    AS5600_i2c_init(i2c1);
    

    // End_switches

    gpio_init(END_SWITCH_1);
    gpio_init(END_SWITCH_2);
    gpio_pull_down(END_SWITCH_1);
    gpio_pull_down(END_SWITCH_2);
    gpio_set_dir(END_SWITCH_1, GPIO_IN);
    gpio_set_dir(END_SWITCH_2, GPIO_IN);

}

static void Motors_dispatch(Motors * const this, 
                            Event const * const e){
    // Initial event
    if(e->sig == INIT_SIG){
        
        // TRIGGER_VOID_EVENT; // Provisional to trigger state machine
         // Do nothing and wait for external signal
    }else{

    // State Machine 
    switch(this->state){
        case MOTORS_AO_CALIB_M1_ST:{        // MOTOR 1 Calibration
            switch(e->sig){
                case MOTORS_AO_START_CALIB_SIG:
                    // Jump to next event response
                case MOTORS_AO_TIMEOUT_SIG:{
                    // Look for the end switch press
                    if(gpio_get(END_SWITCH_1)){
                        this->past_state = MOTORS_AO_CALIB_M1_ST;
                        this->state = MOTORS_AO_CENTER_M1_ST;
                        this->centering_steps = MOTOR1_HOME_TO_CENTER_STEPS;
                        TRIGGER_VOID_EVENT;

                    }else{

                        TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);
                        StepperMotor_move(&(this->motor1), MOTOR1_NEG_DIR,
                                        MOTOR1_CALIB_FREQ, MOTOR1_CALIB_STEPS);
                    }
                    break;
                }default:
                    break;
            }
            break;

        }case MOTORS_AO_CENTER_M1_ST:{      // MOTOR 1 Centering
            switch(e->sig){
                case MOTORS_AO_TIMEOUT_SIG:{
                    if(this->center_m1_state == CENTER_M1_PENDING_ST){
                        if(this->centering_steps < MOTOR1_CENTER_STEPS){
                            if(this->centering_steps != 0){
                                StepperMotor_move(&(this->motor1), MOTOR1_POS_DIR,
                                        MOTOR1_CENTER_FREQ, this->centering_steps);
                            }
                            this->centering_steps = 0;
                            this->center_m1_state = CENTER_M1_DONE_ST;
                            TimeEvent_arm(&this->te, (100 / portTICK_RATE_MS), 0U);

                        }else{
                            TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);
                            StepperMotor_move(&(this->motor1), MOTOR1_POS_DIR,
                                        MOTOR1_CENTER_FREQ, MOTOR1_CENTER_STEPS);
                            this->centering_steps-=MOTOR1_CENTER_STEPS;
                        }

                    }else if(this->center_m1_state == CENTER_M1_DONE_ST){
                        if(this->past_state == MOTORS_AO_CALIB_M1_ST){
                            this->encoder1_zero = read_encoder1();
                            this->encoder1_last_read = this->encoder1_zero;
                            this->state = MOTORS_AO_CALIB_M2_ST;
                        }else{
                            this->state = MOTORS_AO_WAITING_ST;
                        }
                        this->past_state = MOTORS_AO_CENTER_M1_ST;
                        TRIGGER_VOID_EVENT;

                    }
                    break;
                }default:
                    break;
            }
            break;
        }case MOTORS_AO_CALIB_M2_ST:{       // PENDING
            this->past_state = MOTORS_AO_CALIB_M2_ST;
            this->state = MOTORS_AO_CENTER_M2_ST;
            TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U); // PROVISIONAL
            break;
 
        }case MOTORS_AO_CENTER_M2_ST:{      //PENDING
            this->past_state = MOTORS_AO_CENTER_M2_ST;
            this->state = MOTORS_AO_WAITING_ST;
            this->motor1_current_position = 0;
            this->motor2_current_position = 0;
            TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U); // PROVISIONAL
            
            static const Event calibration_ack = {UI_AO_ACK_CALIB_SIG};
            Active_post(AO_blinkyButton, (Event*)&calibration_ack);
            break;
        }case MOTORS_AO_FREE_M1_ST:{
            switch(e->sig){
                case MOTORS_AO_TIMEOUT_SIG:{
                    StepperMotor_disable(&(this->motor1));

                    uint16_t encoder1_current_read = read_encoder1();

                    #if ENCODER1_POS_DIR == 0

                        // Identify overflow
                        if(encoder1_current_read  >= 0 && 
                           encoder1_current_read < 500 && 
                           this->encoder1_last_read <= 4095 &&
                           this->encoder1_last_read > 3595){

                            this->encoder1_turns++;
                            
                        }else if(encoder1_current_read  <= 4095 && 
                                encoder1_current_read > 3595 && 
                                this->encoder1_last_read >= 0 &&
                                this->encoder1_last_read < 500){
                            this->encoder1_turns--;

                        }
                        this->encoder1_current_angle = (int32_t)(3600
                            *(((float)(4095-encoder1_current_read)) 
                            +((float)(this->encoder1_turns*4096))
                            -((float)(4095-this->encoder1_zero))))
                            /(4096*MOTOR1_TRANSMISSION_RATE);


                        this->encoder1_current_angle = 10*MOTOR1_DEG_PER_STEP*
                            ((float)encoder1_current_read) 
                            +((float)(this->encoder1_turns*4096))
                            -((float)this->encoder1_zero);
                    #elif ENCODER1_POS_DIR == 1
                        // Identify overflow
                        if(encoder1_current_read  >= 0 && 
                           encoder1_current_read < 500 && 
                           this->encoder1_last_read <= 4095 &&
                           this->encoder1_last_read > 3595){

                            this->encoder1_turns--;
                            
                        }else if(encoder1_current_read  <= 4095 && 
                                encoder1_current_read > 3595 && 
                                this->encoder1_last_read >= 0 &&
                                this->encoder1_last_read < 500){
                            this->encoder1_turns++;

                        }

                        this->encoder1_current_angle = (int32_t)(3600
                            *(((float)(4095-encoder1_current_read)) 
                            +((float)(this->encoder1_turns*4096))
                            -((float)(4095-this->encoder1_zero))))
                            /(4096*MOTOR1_TRANSMISSION_RATE);
                            

                    #endif

                    this->encoder1_last_read = encoder1_current_read;
                    TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);
                    break;
                }case MOTORS_AO_RQ_DEG_M1_SIG:{
                    static UI_AO_ANGLE_PL calibration_ack = {UI_AO_ACK_DEG_M1_SIG};
                    calibration_ack.angle = this->encoder1_current_angle;
                    Active_post(AO_blinkyButton, (Event*)&calibration_ack);

                    break;
                }case MOTORS_AO_BLOCK_M1_SIG:{
                    StepperMotor_enable(&(this->motor1));
                    this->past_state = MOTORS_AO_FREE_M1_ST;
                    this->state = MOTORS_AO_CENTER_M1_ST;
                    // TODO DIR centering
                    this->centering_steps = this->encoder1_last_read / (10* MOTOR1_DEG_PER_STEP);
                    
                    TRIGGER_VOID_EVENT;


                }
                default:
                    break;
            }
            break;
        }
    
        case MOTORS_AO_WAITING_ST:{
            switch(e->sig){
                case MOTORS_AO_TIMEOUT_SIG:
                    break;

                case MOTORS_AO_MOVE_SIG:{   // TODO: Motion profile precalculation
                    
                    if(((MOTORS_AO_MOVE_PL*)e)->motor == M1){
                        this->motor1_goal_position = 
                                ((float)(((MOTORS_AO_MOVE_PL*)e)->degrees)/10);


                        float steps_to_move = (this->motor1_goal_position -
                            this->motor1_current_position)/MOTOR1_DEG_PER_STEP;

                        if(steps_to_move<0){
                            this->movement_steps = (uint16_t)(-1*steps_to_move);
                            this->movement_dir = MOTOR1_NEG_DIR;
                        }else{
                            this->movement_steps = (uint16_t)(steps_to_move);
                            this->movement_dir = MOTOR1_POS_DIR;
                        }
                        

                        this->state = MOTORS_AO_MOVE_M1_ST;
                        this->past_state = MOTORS_AO_WAITING_ST;
                    }else{
                        this->motor2_goal_position = 
                                ((float)(((MOTORS_AO_MOVE_PL*)e)->degrees)/10);

                        float steps_to_move = (this->motor2_goal_position -
                            this->motor2_current_position)/MOTOR2_DEG_PER_STEP;

                        if(steps_to_move<0){
                            this->movement_steps = (uint16_t)(-1*steps_to_move);
                            this->movement_dir = MOTOR2_CCW_DIR;
                        }else{
                            this->movement_steps = (uint16_t)(steps_to_move);
                            this->movement_dir = MOTOR2_CW_DIR;
                        }

                        this->state = MOTORS_AO_MOVE_M2_ST;
                        this->past_state = MOTORS_AO_WAITING_ST;
                    }
                    TRIGGER_VOID_EVENT;
                    break;
                }case MOTORS_AO_FREE_M1_SIG:{
                    this->state = MOTORS_AO_FREE_M1_ST;
                    this->past_state = MOTORS_AO_WAITING_ST;
                    TRIGGER_VOID_EVENT;
                    break;
                }case MOTORS_AO_FREE_M2_SIG:{
                    this->state = MOTORS_AO_FREE_M2_ST;
                    this->past_state = MOTORS_AO_WAITING_ST;
                    TRIGGER_VOID_EVENT;
                    break;

                }default:
                    break;
            }
            break;
        }case MOTORS_AO_MOVE_M1_ST:{
            switch(e->sig){
                case MOTORS_AO_TIMEOUT_SIG:{
                    if(this->movement_steps<MOTOR1_MOVEMENT_STEPS){
                        if(this->movement_steps != 0){
                            StepperMotor_move(&(this->motor1), this->movement_dir,
                                    MOTOR1_MOVEMENT_FREQ, this->movement_steps);
                        }
                        this->movement_steps = 0;
                        TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);

                        this->state = MOTORS_AO_WAITING_ST;
                        this->past_state = MOTORS_AO_MOVE_M1_ST;
                        static const Event move_m1_ack = {UI_AO_ACK_MOVE_SIG};
                        Active_post(AO_blinkyButton, (Event*)&move_m1_ack);


                    }else{
                        TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);
                        StepperMotor_move(&(this->motor1), this->movement_dir,
                                    MOTOR1_MOVEMENT_FREQ, MOTOR1_MOVEMENT_STEPS);
                        this->movement_steps-=MOTOR1_MOVEMENT_STEPS;
                    }
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

static uint16_t read_encoder1(void){
    
    i2c_init(i2c1, 1000 * 1000);
    AS5600_config_pins(ENCODER1_SDA_PIN,ENCODER1_SCL_PIN);
    AS5600_config_pull_up(ENCODER1_SDA_PIN, ENCODER1_SCL_PIN);
    uint16_t reading = AS5600_read_angle(i2c1);
    AS5600_free_pins(ENCODER1_SDA_PIN,ENCODER1_SCL_PIN);

    return reading;
}

static uint16_t read_encoder2(void){
    
    i2c_init(i2c1, 1000 * 1000);
    AS5600_config_pins(ENCODER2_SDA_PIN,ENCODER2_SCL_PIN);
    AS5600_config_pull_up(ENCODER2_SDA_PIN, ENCODER2_SCL_PIN);
    uint16_t reading = AS5600_read_angle(i2c1);
    AS5600_free_pins(ENCODER2_SDA_PIN,ENCODER2_SCL_PIN);
    return reading;
}