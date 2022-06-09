
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

// Project libraries
#include "pio_stepper.h"
#include "AS5600.h"

// Another AO /

#include "UI_AO.h" 


#define TRIGGER_VOID_EVENT TimeEvent_arm(&this->te, (1 / portTICK_RATE_MS), 0U)


int16_t angle_counter = 0; //Just to imitate the behaviour


void Motors_ctor(Motors * const this){
    Active_ctor(&this->super, (DispatchHandler)&Motors_dispatch);
    
    // Time events construction
    TimeEvent_ctor(&this->te, MOTORS_AO_TIMEOUT_SIG, &this->super);
    
    // State Machine initialization
    this->state = MOTORS_AO_CALIB_M1_ST;
    this->past_state = MOTORS_AO_CALIB_M1_ST;
    this->centering_steps = 0;
    this->movement_steps = 0;
    this->motor1_current_position = 0;
    this->motor2_current_position = 0;
    this->motor1_goal_position = 0;
    this->motor2_goal_position = 0;
    this->movement_dir = false;

    // private data initialization


    // Init code, preferably use bsp.c defined functions to control peripheral 
    // to keep encapsulation

    // StepperMotor_ctor(&(this->motor1), pio0, 0, MOTOR1_DIR_PIN, 
    //               MOTOR1_STEP_PIN, MOTOR1_ENABLE_PIN, 400, 3);
    // StepperMotor_ctor(&(this->motor2), pio1, 0, MOTOR2_DIR_PIN, 
    //               MOTOR2_STEP_PIN, MOTOR2_ENABLE_PIN, 400, 1);
    // AS5600_i2c_init(i2c1);           

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
    }
    else{

    // State Machine 
    switch(this->state){
        case MOTORS_AO_CALIB_M1_ST:{
            switch(e->sig){
                case MOTORS_AO_START_CALIB_SIG:{
                    printf("Start calib sig from UI was received\n");
                    TimeEvent_arm(&this->te, (2000 / portTICK_RATE_MS), 0U);
                break;
                }
                case MOTORS_AO_TIMEOUT_SIG:{                    
                    static Event ack_calib_event = {UI_AO_ACK_CALIB_SIG};
                    Active_post(AO_UI, (Event*)&ack_calib_event);
                    this->state = MOTORS_AO_WAITING_ST;             
                break;
                }
                default:
                    break;
            }
            break;

        }
        case MOTORS_AO_WAITING_ST:{
            switch(e->sig){
                case MOTORS_AO_MOVE_SIG:{   // Motion profile precalculation
                    if(((MOTORS_AO_MOVE_PL*)e)->motor == M1){
                        printf("Moving motor one (base):\n");
                        this->motor1_goal_position = ((MOTORS_AO_MOVE_PL*)e)->degrees;
                        printf("Pos: %f\n", this->motor1_goal_position);
                    }else{
                        printf("Moving motor two (manilar):\n");
                        this->motor2_goal_position = ((MOTORS_AO_MOVE_PL*)e)->degrees;
                        printf("Pos: %f\n", this->motor2_goal_position);
                    }
                    TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U); // PROVISIONAL                    
                    break;
                }
                case MOTORS_AO_TIMEOUT_SIG:{
                    static Event ack_move_event = {UI_AO_ACK_MOVE_SIG};
                    Active_post(AO_UI, (Event*)&ack_move_event);
                break;
                }
                case MOTORS_AO_FREE_M1_SIG:{
                    this->state = MOTORS_AO_FREE_M1_ST;
                break;
                }
                case MOTORS_AO_FREE_M2_SIG:{
                    this->state = MOTORS_AO_FREE_M2_ST;
                break;
                }                
                default:
                    break;
            }
            break;
        }
        case MOTORS_AO_FREE_M2_ST:{
            switch(e->sig){
                case MOTORS_AO_RQ_DEG_M2_SIG:{
                    printf("rq angle M2 from ui received\n");
                    angle_counter++;
                    static UI_AO_ANGLE_PL angle_event_M2 = {UI_AO_ACK_DEG_M2_SIG};
                    angle_event_M2.angle = angle_counter;
                    Active_post(AO_UI, (Event*)&angle_event_M2);
                break;
                }
                case MOTORS_AO_BLOCK_M2_SIG:{
                    angle_counter = 0;
                    printf("block signal M2 from ui received\n");
                    this->state = MOTORS_AO_WAITING_ST;
                break;
                }
                default:
                    break;
            }
        break;
        }
        case MOTORS_AO_FREE_M1_ST:{
            switch(e->sig){
                case MOTORS_AO_RQ_DEG_M1_SIG:{
                    printf("rq angle M1 from ui received\n");
                    angle_counter++;
                    static UI_AO_ANGLE_PL angle_event_M1 = {UI_AO_ACK_DEG_M1_SIG};
                    angle_event_M1.angle = angle_counter;
                    Active_post(AO_UI, (Event*)&angle_event_M1);
                break;
                }
                case MOTORS_AO_BLOCK_M1_SIG:{
                    angle_counter = 0;
                    printf("block signal M1 from ui received\n");
                    this->state = MOTORS_AO_WAITING_ST;
                break;
                }
                default:
                    break;
            }
        break;
        }
        
        
        default:
            break;
    }
    }
}

// static void Motors_dispatch(Motors * const this, 
//                             Event const * const e){
//     // Initial event
//     if(e->sig == INIT_SIG){
        
//         TRIGGER_VOID_EVENT; // PROVISIONAL
//          // Do nothing and wait for external signal
//     }else{

//     // State Machine 
//     switch(this->state){
//         case MOTORS_AO_CALIB_M1_ST:{
//             switch(e->sig){
//                 case MOTORS_AO_START_CALIB_SIG:
//                 case MOTORS_AO_TIMEOUT_SIG:{
//                     if(gpio_get(END_SWITCH_1)){
//                         this->past_state = MOTORS_AO_CALIB_M1_ST;
//                         this->state = MOTORS_AO_CENTER_M1_ST;
//                         TRIGGER_VOID_EVENT;
//                         this->encoder1_zero = read_encoder1();

//                         this->centering_steps = MOTOR1_FULL_RANGE_STEPS / 2;

//                     }else{

//                         // TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);
//                         // StepperMotor_move(&(this->motor1), MOTOR1_CCW_DIR,
//                         //                 MOTOR1_CALIB_FREQ, MOTOR1_CALIB_STEPS);
//                     }
//                     break;
//                 }default:
//                     break;
//             }
//             break;

//         }case MOTORS_AO_CENTER_M1_ST:{
//             switch(e->sig){
//                 case MOTORS_AO_TIMEOUT_SIG:{
//                     if(this->centering_steps<MOTOR1_CENTER_STEPS){
//                         // if(this->centering_steps != 0){
//                         //     StepperMotor_move(&(this->motor1), MOTOR1_CW_DIR,
//                         //             MOTOR1_CENTER_FREQ, this->centering_steps);
//                         // }
//                         this->centering_steps = 0;
//                         TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);

//                         if(this->past_state == MOTORS_AO_CALIB_M1_ST){
//                             this->state = MOTORS_AO_CALIB_M2_ST;
//                         }else{
//                             this->state = MOTORS_AO_WAITING_ST;
//                         }
//                         this->past_state = MOTORS_AO_CENTER_M1_ST;

//                     }else{
//                         // TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);
//                         // StepperMotor_move(&(this->motor1), MOTOR1_CW_DIR,
//                         //             MOTOR1_CENTER_FREQ, MOTOR1_CENTER_STEPS);
//                         // this->centering_steps-=MOTOR1_CENTER_STEPS;
//                     }
//                     break;
                    
//                 }default:
//                     break;
//             }
//             break;
//         }case MOTORS_AO_CALIB_M2_ST:        // PENDING
//             this->past_state = MOTORS_AO_CALIB_M2_ST;
//             this->state = MOTORS_AO_CENTER_M2_ST;
//             TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U); // PROVISIONAL
//             break;
 
//         case MOTORS_AO_CENTER_M2_ST:        //PENDING
//             this->past_state = MOTORS_AO_CENTER_M2_ST;
//             this->state = MOTORS_AO_WAITING_ST;
//             this->motor1_current_position = 0;
//             this->motor2_current_position = 0;
//             TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U); // PROVISIONAL
//             break;
//         case MOTORS_AO_WAITING_ST:{
//             switch(e->sig){
//                 case MOTORS_AO_TIMEOUT_SIG:

//                 case MOTORS_AO_MOVE_SIG:{   // Motion profile precalculation
                    
//                     if(((MOTORS_AO_MOVE_PL*)e)->motor == M1){
//                         this->motor1_goal_position = 
//                                 ((float)(((MOTORS_AO_MOVE_PL*)e)->degrees)/10);


//                         int16_t signed_value = (uint16_t)((this->motor1_goal_position -
//                             this->motor1_current_position)/MOTOR1_DEGREES_PER_STEPS);

//                         if(signed_value<0){
//                             this->movement_steps = (uint16_t)(-1*signed_value);
//                             this->movement_dir = MOTOR1_CCW_DIR;
//                         }else{
//                             this->movement_steps = (uint16_t)(signed_value);
//                             this->movement_dir = MOTOR1_CW_DIR;
//                         }
                        

//                         this->state = MOTORS_AO_MOVE_M1_ST;
//                         this->past_state = MOTORS_AO_WAITING_ST;
//                     }else{
//                         this->motor2_goal_position = ((MOTORS_AO_MOVE_PL*)e)->degrees;

//                         int16_t signed_value = (uint16_t)((this->motor2_goal_position -
//                             this->motor2_current_position)/MOTOR2_DEGREES_PER_STEPS);

//                         if(signed_value<0){
//                             this->movement_steps = (uint16_t)(-1*signed_value);
//                             this->movement_dir = MOTOR2_CCW_DIR;
//                         }else{
//                             this->movement_steps = (uint16_t)(signed_value);
//                             this->movement_dir = MOTOR2_CW_DIR;
//                         }

//                         this->state = MOTORS_AO_MOVE_M2_ST;
//                         this->past_state = MOTORS_AO_WAITING_ST;
//                     }                    
//                     break;

//                 }default:
//                     break;
//             }
//             break;
//         }case MOTORS_AO_MOVE_M1_ST:{
//             switch(e->sig){
//                 case MOTORS_AO_TIMEOUT_SIG:{
//                     if(this->movement_steps<MOTOR1_MOVEMENT_STEPS){
//                         // if(this->movement_steps != 0){
//                         //     StepperMotor_move(&(this->motor1), this->movement_dir,
//                         //             MOTOR2_MOVEMENT_FREQ, this->centering_steps);
//                         // }
//                         this->movement_steps = 0;
//                         TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);

//                         this->state = MOTORS_AO_WAITING_ST;
//                         this->past_state = MOTORS_AO_MOVE_M1_ST;

//                     }else{
//                         // TimeEvent_arm(&this->te, (10 / portTICK_RATE_MS), 0U);
//                         // StepperMotor_move(&(this->motor1), this->movement_dir,
//                         //             MOTOR2_MOVEMENT_FREQ, MOTOR1_MOVEMENT_STEPS);
//                         // this->movement_steps-=MOTOR1_MOVEMENT_STEPS;
//                     }
//                     break;
                    
//                 }default:
//                     break;
//             }
//             break;           

//         }default:
//             break;
//     }
//     }
// }

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