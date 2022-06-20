/** 
  ******************************************************************************
  * @file    UI_AO.c
  * @author  Camilo Vera
  * @brief   blinky active object
  *          This file constainst an implentation example of an active object
  *          using FreeAct over FreeRTOS 
  ****************************************************************************** 
*/

/* Includes ------------------------------------------------------------------*/
#include "UI_AO.h"

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

// Another AO //añado a quien le envío eventos

#include "printer_AO.h"
#include "Motors_AO.h" 

// Project libraries
#include "bsp.h"

#define TRIGGER_VOID_EVENT TimeEvent_arm(&this->te, (1 / portTICK_RATE_MS), 0U)

/* Implementation ------------------------------------------------------------*/



// Global UI_States variables
static UI_State HOME = {" ***  Welcome!  *** "};
static UI_State CALIBRATE = {"    Calibrating...  "};
static UI_State INICIO = {" Choose an option:  ", 2,{" Create Routine     ", " Do default routine "}};
static UI_State CREATE = {" Add exercise       ", 4, {" PronoSupination    ", " FlexoExtension     ", " Ab-,Adduction      ", " Begin Routine      "}};
static UI_State DO_DEFAULT = {" Default routine:   ", 2, {" Check Routine first", " Do routine now     "}};
static UI_State CONFIG_EXERCISE = {"Config. ", 5, {" Repetitions: 1     ", " Min. Angle: -20    ", " Max. Angle: 20     ", " Time: 3            ", " Exercise Ready     "}};
static UI_State BEGIN_ROUTINE = {" Created routine    ", 3,{" Check Routine first", " Do routine now     ", " Set pause betw. ex."}};
static UI_State CHECK_ROUTINE = {"Check routine:      ", 2, {" See exercises      ", " See pause betw. ex."}};
static UI_State SHOW_EXERCISES = {"See exercises:      ", 10, {" Exercise 1         ", " Exercise 2         ", " Exercise 3         ", " Exercise 4         ",
                                    " Exercise 5         ", " Exercise 6         ", " Exercise 7         ", " Exercise 8         ", " Exercise 9         ", " Exercise 10        "}};
static UI_State SHOW_AN_EXERCISE = {" Exercise           ", 5, {" Type:              ", " Repetitions:       ", " Min. Angle:        ", " Max. Angle:        ", " Time:              "}};

//Global iterator
uint i = 0;

//Global buffer for LCD screen
char modified_buffer[20];

//select min or max angle
uint angle = 0; //o: min, 1:max

char availableExercises[3][12] = {"PronoSup.", "FlexoExt.", "Ab-,Adduc."};

//previous parameter values
int8_t prev_reps;
int8_t prev_time;
int8_t prev_pause;

//new angle from encoder
int16_t new_angle = 5;

//counter
int8_t counter = 3;

//used to save numerical value in char value
char char_data[3];

//error messages
char error_message1[20];
char error_message2[20];

//pause active
bool pause_active = false;

//Exercises for default routine:
Exercise default_exercise_1 = {0, 3, -400, 400, 3};
Exercise default_exercise_2 = {1, 3, -400, 400, 3};
Exercise default_exercise_3 = {2, 3, -400, 400, 3};

//Selected routine. 0: default routine, 1: created routine
uint selected_routine = 0;

//number of exercise to see or execute
uint selected_exercise = 0;

uint current_repetition = 0;
bool inicio;
bool inExercise = false;

//Routines
Routine created_routine;
Routine default_routine;
Routine routine_to_do;

int16_t degrees_to_send = 0; 


/* AO Class Constructor ------------------------------------------------------*/
/**
 * @brief This function implements the initialization of the AO, and is the 
 * proper place to execute peripheral initialization, initial states definition 
 * and assignation of variable initial values, as long as user input isn't 
 * required.
 *
 * @param this Object instance
 */


void UI_ctor(UI * const this){

    Active_ctor(&this->super, (DispatchHandler)&UI_dispatch);
    
    TimeEvent_ctor(&this->te, UI_AO_TIMEOUT_SIG, &this->super);

    //initial state
    this->state = UI_AO_HOME_ST;

    // default values
    this->default_reps = 1;
    this->default_min_angle = -200;
    this->default_max_angle = 200;
    this->default_time_in_position = 3;
    this->default_pause_between_exercises = 5;

    // // limit values for exercise parameters
    this->max_reps = 50;
    this->max_secs = 100;
    this->max_pause = 100;

    //initialize exercise parameters
    this->reps = this->default_reps;
    this->min_angle = this->default_min_angle;
    this->max_angle = this->default_max_angle;
    this->time_in_position = this->default_time_in_position;
    this->pause_between_exercises = this->default_pause_between_exercises;


    //exercise type PronoSup at the beginning:
    this->exercise_type = 0;

    //initial parameters for created routine
    created_routine.num_ejercicios = 0;
    created_routine.pause = this->pause_between_exercises;

    //initial parameters for default routine
    default_routine.num_ejercicios = 3;
    default_routine.pause = this->pause_between_exercises;
    default_routine.ejercicios[0] = default_exercise_1;
    default_routine.ejercicios[1] = default_exercise_2;
    default_routine.ejercicios[2] = default_exercise_3;
    
        
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
static void UI_dispatch(UI * const this, //dispatch se ejecuta siempre
                                  Event const * const e){

    if(e->sig == INIT_SIG){ 
        TRIGGER_VOID_EVENT;
        
    }else{
        if(e->sig == UI_AO_ERROR_SIG){
            change_string(modified_buffer, 0, ((UI_AO_ERROR_PL*)e)->error_message);            
            display_rows("    Error occured   ", modified_buffer, "                    ", " Restart device     ");
            inExercise = false;
            this->state = UI_AO_ERROR_ST;
        }
        if(e->sig == UI_AO_SW5_PRESSED_SIG){
            if(inExercise){
                if(!pause_active){
                    this->state = UI_AO_PAUSE_ST;
                    pause_active = true;
                }
                else{
                    this->state = UI_AO_CENTER_DEVICE_ST;
                    TRIGGER_VOID_EVENT;
                    pause_active = false;
                }
            }
        }
        switch(this->state){

            case UI_AO_PAUSE_ST:{
                display_rows("System was          ", "paused              ", "                    ", "                    ");              
            break;
            }
            case UI_AO_ERROR_ST:{                             
            break;
            }
            case UI_AO_HOME_ST:{

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{
                        this->state = UI_AO_REMOVE_HANDS_ST;
                        display_row2(HOME.title);
                        TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U);
                    break;
                    }default:
                        break;                
                }
            break;
            }
            case UI_AO_REMOVE_HANDS_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        if(counter == 0){
                            change_string(modified_buffer, 0, "Now!");
                            display_row4(modified_buffer);
                            counter = 3;
                            this->state = UI_AO_CALIBRATE_ST;
                            TimeEvent_arm(&this->te, (1500 / portTICK_RATE_MS), 0U);
                        }
                        else{
                        sprintf(char_data,"%ld", counter);
                        change_string(modified_buffer, 0, char_data);
                        display_rows("   Take hands off   ", "     the device     ", "   Calibrating in   ", modified_buffer);
                        counter--;
                        TimeEvent_arm(&this->te, (1000 / portTICK_RATE_MS), 0U);
                        }                        
                        break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_CALIBRATE_ST:{

                switch (e->sig){

                    case UI_AO_TIMEOUT_SIG:{
                        display_rows("                    ", CALIBRATE.title, "                    ", "                    ");                       
                        static Event calib_event = {MOTORS_AO_START_CALIB_SIG};
                        Active_post(AO_Motors, (Event*)&calib_event);                        
                        break;
                    }
                    case UI_AO_ACK_CALIB_SIG:{
                        printf("ACK from motors was received :D");
                        this->state = UI_AO_INICIO_ST;
                        TRIGGER_VOID_EVENT; 
                    break;
                    }                    
                    default:
                        break;
                }
            break;
            }
            case UI_AO_INICIO_ST:{   

                switch (e->sig){ 
                            
                    case UI_AO_TIMEOUT_SIG:{
                        display_inicio(INICIO);
                    break;
                    }

                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(INICIO); 
                    break;
                    }

                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(INICIO); 
                    break;
                    }

                    case UI_AO_SW4_PRESSED_SIG:{
                        this->state = (i == 0) ? UI_AO_CREATE_ST : UI_AO_DO_DEFAULT_ST;               
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_CREATE_ST:{

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{
                        display_inicio(CREATE);
                    break;
                    }

                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(CREATE);                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(CREATE);
                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        this->state = (i == 3) ? UI_AO_BEGIN_ROUTINE_ST : UI_AO_CONFIG_EXERCISE_ST;               
                        this->exercise_type = i;
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_INICIO_ST;               
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }
                    default:
                        break;                
                }
            break;
            }
            case UI_AO_CONFIG_EXERCISE_ST:{

                switch (e->sig){
                
                    case UI_AO_TIMEOUT_SIG:{                    
                        change_string(CONFIG_EXERCISE.title, 8, availableExercises[this->exercise_type]);
                        display_inicio(CONFIG_EXERCISE);    
                                                        
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(CONFIG_EXERCISE);                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(CONFIG_EXERCISE);                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        switch (i){
                            case 0:{
                                this->state = UI_AO_REPETITIONS_ST;
                            break;
                            }
                            case 1:{
                                angle = 0;
                                this->state = UI_AO_POS_BAR_ST;
                            break;
                            }
                            case 2:{
                                angle = 1;
                                this->state = UI_AO_POS_BAR_ST;
                            break;
                            }
                            case 3:{
                                this->state = UI_AO_TIME_ST;
                            break;
                            }
                            case 4:{
                                this->state = UI_AO_EXERCISE_READY_ST;
                            break;
                            }
                            default:
                                break;
                        }
                        i = 0;
                        TRIGGER_VOID_EVENT;                                        
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_CREATE_ST;               
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }                    
                    default:
                        break;
                }
            break;
            }
            case UI_AO_REPETITIONS_ST:{              

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{                        
                        prev_reps = this->reps;
                        sprintf(char_data,"%ld", this->reps);
                        change_string(modified_buffer, 0, char_data);
                        display_rows("   Set number of    ", "    repetitions:    ", "--------------------", modified_buffer);                       
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        if(this->reps <this->max_reps){
                            this->reps++;
                            sprintf(char_data,"%ld", this->reps);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);                            
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        if(this->reps > 1){
                            this->reps--;
                            sprintf(char_data,"%ld", this->reps);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        sprintf(char_data,"%ld", this->reps);
                        change_string(CONFIG_EXERCISE.options[0], 14, char_data);
                        i = 0;
                        this->state = UI_AO_CONFIG_EXERCISE_ST; 
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->reps = prev_reps;
                        this->state = UI_AO_CONFIG_EXERCISE_ST;               
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_POS_BAR_ST:{             

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{
                        if(this->exercise_type == 0 || this-> exercise_type == 1){

                            static MOTORS_AO_MOVE_PL vertical_movement_event = {MOTORS_AO_MOVE_SIG, M2, 0};
                            Active_post(AO_Motors, (Event*)&vertical_movement_event);
                            display_rows("    Positioning     ", "        bar         ", "     vertically     ", "                    ");
                        }                        
                        else if(this->exercise_type == 2){
                            static MOTORS_AO_MOVE_PL horizontal_movement_event = {MOTORS_AO_MOVE_SIG, M2, -900};
                            Active_post(AO_Motors, (Event*)&horizontal_movement_event);
                            display_rows("    Positioning     ", "         bar        ", "    horizontally    ", "                    ");
                        }
                    break;
                    }                      
                    case UI_AO_ACK_MOVE_SIG:{
                        printf("ACK move from motors received\n");
                        this->state = UI_AO_MEASURE_ANGLE_ST;
                        TRIGGER_VOID_EVENT; 
                        break;
                    }  
                    default:
                        break;
                }
            break;
            }
            case UI_AO_MEASURE_ANGLE_ST:{             
                
                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{ 
                        display_rows(" Move to min. angle ", "if ready press enter", "--------------------", "                    ");                       
                        if(angle == 0){
                            display_row1(" Move to min. angle ");                            
                        }
                        else if(angle == 1){
                            display_row1(" Move to max. angle ");
                        }
                        
                        if(this->exercise_type == 0){                            
                            // Liberamos motor de la manivela
                            static Event free_motor2_event = {MOTORS_AO_FREE_M2_SIG};
                            Active_post(AO_Motors, (Event*)&free_motor2_event);      
                            
                            // Request para el ángulo del disco
                            static Event rq_motor2_event = {MOTORS_AO_RQ_DEG_M2_SIG};
                            Active_post(AO_Motors, (Event*)&rq_motor2_event);       
                        }
                        else{                            
                            // Liberamos motor de la base
                            static Event free_motor1_event = {MOTORS_AO_FREE_M1_SIG};
                            Active_post(AO_Motors, (Event*)&free_motor1_event);  

                            //Request para el ángulo de la base
                            static Event rq_motor1_event = {MOTORS_AO_RQ_DEG_M1_SIG};
                            Active_post(AO_Motors, (Event*)&rq_motor1_event); 
                        }
                    break;
                    }      
                    case UI_AO_ACK_DEG_M1_SIG:{
                        printf("Ack received from motors deg M1\n");
                        new_angle = (((UI_AO_ANGLE_PL*)e)->angle);
                        printf("Angle received M1: %d", new_angle);
                        sprintf(char_data,"%ld", new_angle/10);
                        change_string(modified_buffer, 0, char_data);
                        display_row4(modified_buffer);
                        vTaskDelay(50 / portTICK_PERIOD_MS);
                        static Event rq_motor1_event2 = {MOTORS_AO_RQ_DEG_M1_SIG};
                        Active_post(AO_Motors, (Event*)&rq_motor1_event2);   
                                            
                    break;
                    }   
                    case UI_AO_ACK_DEG_M2_SIG:{
                        printf("Ack received from motors deg M2\n");
                        new_angle = (((UI_AO_ANGLE_PL*)e)->angle);
                        printf("Angle received M2: %d", new_angle);
                        sprintf(char_data,"%ld", new_angle/10);
                        change_string(modified_buffer, 0, char_data);
                        display_row4(modified_buffer);
                        vTaskDelay(50 / portTICK_PERIOD_MS);
                        static Event rq_motor2_event2 = {MOTORS_AO_RQ_DEG_M2_SIG};
                        Active_post(AO_Motors, (Event*)&rq_motor2_event2);                          
                    break;
                    }           
                    
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("Enter pressed\nAngle to save: %d\n", new_angle);
                        if(angle == 0){                            
                            this->min_angle = new_angle;
                            sprintf(char_data,"%ld", this->min_angle/10);
                            change_string(CONFIG_EXERCISE.options[1], 13, char_data);
                        }
                        else if(angle == 1){
                            this->max_angle = new_angle;
                            sprintf(char_data,"%ld", this->max_angle/10);
                            change_string(CONFIG_EXERCISE.options[2], 13, char_data);
                        }
                        if(this->exercise_type == 0){
                            static Event block_motor2_event = {MOTORS_AO_BLOCK_M2_SIG};
                            Active_post(AO_Motors, (Event*)&block_motor2_event);                        
                        }
                        else{
                            static Event block_motor1_event = {MOTORS_AO_BLOCK_M1_SIG};
                            Active_post(AO_Motors, (Event*)&block_motor1_event);  
                        }
                        this->state = UI_AO_CONFIG_EXERCISE_ST; 
                        TRIGGER_VOID_EVENT;                                          
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_CONFIG_EXERCISE_ST; 
                        if(this->exercise_type == 0){
                            static Event block_motor2_event2 = {MOTORS_AO_BLOCK_M2_SIG};
                            Active_post(AO_Motors, (Event*)&block_motor2_event2);                        
                        }
                        else{
                            static Event block_motor1_event2 = {MOTORS_AO_BLOCK_M1_SIG};
                            Active_post(AO_Motors, (Event*)&block_motor1_event2);  
                        }                                      
                        i = 0;                        
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    } 
                    default:
                        break; 
                }
            break;
            }    
            case UI_AO_TIME_ST:{              

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{                        
                        prev_time = this->time_in_position;
                        sprintf(char_data,"%ld", this->time_in_position);
                        change_string(modified_buffer, 0, char_data);
                        display_rows("    Set time in     ", "      seconds:      ", "--------------------", modified_buffer);
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        if(this->time_in_position <this->max_secs){
                            this->time_in_position++;
                            sprintf(char_data,"%ld", this->time_in_position);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        if(this->time_in_position > 1){
                            this->time_in_position--;
                            sprintf(char_data,"%ld", this->time_in_position);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }                      
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        sprintf(char_data,"%ld", this->time_in_position);
                        change_string(CONFIG_EXERCISE.options[3], 7, char_data);
                        i = 0;
                        this->state = UI_AO_CONFIG_EXERCISE_ST; 
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->time_in_position = prev_time;
                        this->state = UI_AO_CONFIG_EXERCISE_ST;               
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }
                    default:
                        break;
                }
            break;
            }

            case UI_AO_EXERCISE_READY_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{ 
                        if(created_routine.num_ejercicios<10){ 
                            if(this->min_angle < this->max_angle){
                                Exercise new_exercise = {this->exercise_type, this->reps, this->min_angle,
                                                        this->max_angle, this->time_in_position};
                                created_routine.ejercicios[created_routine.num_ejercicios] = new_exercise;
                                created_routine.num_ejercicios++;
                                change_string(modified_buffer, 0, "                    ");
                                change_string(modified_buffer, 5, availableExercises[this->exercise_type]);
                                display_rows("    Exercise of     ", modified_buffer, "    was  added      ", "                    ");   
                                this->state = UI_AO_CREATE_ST;  
                                TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U); 
                                for(int i = 0; i<created_routine.num_ejercicios; i++){
                                    printf("Exercise %d:\n", i);
                                    printf("tipo: %d\n",created_routine.ejercicios[i].type_of_exercise);
                                    printf("reps: %d\n",created_routine.ejercicios[i].num_of_reps);
                                    printf("min: %d\n",created_routine.ejercicios[i].lim_min);
                                    printf("max: %d\n",created_routine.ejercicios[i].lim_max);
                                    printf("secs: %d\n",created_routine.ejercicios[i].time_pos);
                                }                       
                            }
                            else{
                                display_rows("   Max. angle must  ", "   be greater than  ", "     min. angle     ", "                    ");
                                this->state = UI_AO_CONFIG_EXERCISE_ST;
                                TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U);                           
                            } 
                        }  
                        else{
                            display_rows("   Max. number of   ", "     exercises      ", "     was reached    ", "-Begin routine now!-");
                            this->state = UI_AO_CREATE_ST;  
                            TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U); 
                        }                   
                        
                    break;
                    }
                    default:
                        break;

                }

            break;    
            }

            case UI_AO_BEGIN_ROUTINE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        if(created_routine.num_ejercicios == 0){
                            display_rows("  Routine is empty  ", "                    ", "Please add exercises", "                    ");
                            this->state = UI_AO_CREATE_ST;
                            TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U);
                        }
                        else{
                            routine_to_do = created_routine;
                            display_inicio(BEGIN_ROUTINE);
                        }
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(BEGIN_ROUTINE);                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(BEGIN_ROUTINE);                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        selected_routine = 1;
                        printf("Enter\n");
                        switch (i){
                            case 0:{                                
                                this->state = UI_AO_CHECK_ROUTINE_ST;
                            break;
                            }
                            case 1:{
                                this->state = UI_AO_DO_ROUTINE_NOW_ST;
                            break;
                            }
                            case 2:{
                                this->state = UI_AO_SET_PAUSE_ST;
                            break;
                            }
                            default:
                                break;
                        }
                    i = 0; 
                    TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_CREATE_ST;
                        i = 0;         
                        TRIGGER_VOID_EVENT;                            
                    break;
                    }
                    default:
                        break;

                }

            break;
            }

            case UI_AO_SET_PAUSE_ST:{

                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{                        
                        prev_pause = this->pause_between_exercises;
                        sprintf(char_data,"%ld", this->pause_between_exercises);
                        change_string(modified_buffer, 0, char_data);
                        display_rows(" Set pause between  ", " exercises in secs: ", "--------------------", modified_buffer);
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        if(this->pause_between_exercises <this->max_pause){
                            this->pause_between_exercises++;
                            sprintf(char_data,"%ld", this->pause_between_exercises);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        if(this->pause_between_exercises > 1){
                            this->pause_between_exercises--;
                            sprintf(char_data,"%ld", this->pause_between_exercises);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }                      
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        i = 0;
                        created_routine.pause = this->pause_between_exercises;
                        display_rows("   Pause  between   ", " exercises was set. ", "                    ", "                    ");
                        this->state = UI_AO_BEGIN_ROUTINE_ST; 
                        TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U);                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->pause_between_exercises = prev_pause;
                        this->state = UI_AO_BEGIN_ROUTINE_ST;               
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }


                    default:
                        break;
                }
                break;
            }
            case UI_AO_DO_DEFAULT_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{                        
                        routine_to_do = default_routine;
                        display_inicio(DO_DEFAULT);                        
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(DO_DEFAULT);                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(DO_DEFAULT);                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        selected_routine = 0;
                        switch (i){
                            case 0:{                                
                                this->state = UI_AO_CHECK_ROUTINE_ST;
                            break;
                            }
                            case 1:{
                                this->state = UI_AO_DO_ROUTINE_NOW_ST;
                            break;
                            }
                            default:
                                break;
                        }
                    i = 0; 
                    TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_INICIO_ST;
                        i = 0;         
                        TRIGGER_VOID_EVENT;                            
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_CHECK_ROUTINE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        display_inicio(CHECK_ROUTINE);                        
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(CHECK_ROUTINE);                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(CHECK_ROUTINE);                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        switch (i){
                            case 0:{                                
                                this->state = UI_AO_SEE_EXERCISES_ST;
                            break;
                            }
                            case 1:{
                                this->state = UI_AO_SEE_PAUSE_ST;
                            break;
                            }
                            default:
                                break;
                        }
                    i = 0; 
                    TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = (selected_routine == 0) ? UI_AO_DO_DEFAULT_ST : UI_AO_BEGIN_ROUTINE_ST;  
                        i = 0;         
                        TRIGGER_VOID_EVENT;                            
                    break;
                    }                  
                    default:
                        break;
                }
            break;
            }
            case UI_AO_SEE_PAUSE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        sprintf(char_data,"%ld", routine_to_do.pause);
                        change_string(modified_buffer, 0, char_data);
                        display_rows("   Pause  between   ","    exercises in    ", "      seconds:      ", modified_buffer);                   
                    break;
                    }
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_CHECK_ROUTINE_ST;
                        TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_SEE_EXERCISES_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        SHOW_EXERCISES.num_of_options = routine_to_do.num_ejercicios;
                        if(routine_to_do.num_ejercicios < 2){
                            display_rows("See exercises:      ", "--------------------", "*Exercise 1         ", "                    ");
                        }
                        else{
                            display_inicio(SHOW_EXERCISES);
                        }               
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(SHOW_EXERCISES);                                         
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(SHOW_EXERCISES);                                         
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        selected_exercise = i;
                        this->state = UI_AO_SEE_AN_EXERCISE_ST;
                        i = 0;
                        TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_CHECK_ROUTINE_ST;
                        i = 0;
                        TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_SEE_AN_EXERCISE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        sprintf(char_data,"%ld", selected_exercise + 1);
                        change_string(SHOW_AN_EXERCISE.title, 10, char_data);
                        change_string(SHOW_AN_EXERCISE.options[0], 7, availableExercises[routine_to_do.ejercicios[selected_exercise].type_of_exercise]);
                        sprintf(char_data,"%ld", routine_to_do.ejercicios[selected_exercise].num_of_reps);
                        change_string(SHOW_AN_EXERCISE.options[1], 14, char_data);
                        sprintf(char_data,"%ld", routine_to_do.ejercicios[selected_exercise].lim_min/10);
                        change_string(SHOW_AN_EXERCISE.options[2], 13, char_data);
                        sprintf(char_data,"%ld", routine_to_do.ejercicios[selected_exercise].lim_max/10);
                        change_string(SHOW_AN_EXERCISE.options[3], 13, char_data);
                        sprintf(char_data,"%ld", routine_to_do.ejercicios[selected_exercise].time_pos);
                        change_string(SHOW_AN_EXERCISE.options[4], 7, char_data);
                        display_inicio(SHOW_AN_EXERCISE);
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        display_plus_button(SHOW_AN_EXERCISE);                                         
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        display_minus_button(SHOW_AN_EXERCISE);                                         
                    break;
                    }
                    case UI_AO_SW1_PRESSED_SIG:{
                        this->state = UI_AO_SEE_EXERCISES_ST;
                        i = 0;
                        TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_DO_ROUTINE_NOW_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        selected_exercise = 0;
                        display_rows("   Take bar  with   ", "   your left hand   ", "                    ", "If ready press enter");
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        this->state = UI_AO_COUNTDOWN_ST;
                        TRIGGER_VOID_EVENT;
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_COUNTDOWN_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        if(counter == 0){
                            change_string(modified_buffer, 0, "GO!");
                            display_row4(modified_buffer);
                            this->state = UI_AO_MOVE_BAR_ST;
                            TimeEvent_arm(&this->te, (1500 / portTICK_RATE_MS), 0U);
                        }
                        else{
                        sprintf(char_data,"%ld", counter);
                        change_string(modified_buffer, 0, char_data);
                        display_rows("                    ", "Beginning routine in", "                    ", modified_buffer);
                        counter--;
                        TimeEvent_arm(&this->te, (1000 / portTICK_RATE_MS), 0U);
                        }                        
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
             case UI_AO_CENTER_DEVICE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        display_rows("      Centering     ", "       device       ", "                    ", "                    ");
                        static MOTORS_AO_MOVE_PL center_movement_event = {MOTORS_AO_MOVE_SIG, M1, 0};
                        Active_post(AO_Motors, (Event*)&center_movement_event);                     
                        inExercise = true;
                    break;
                    }
                    case UI_AO_ACK_MOVE_SIG:{
                        printf("ACK move center from motors received\n");
                        this->state = UI_AO_END_OF_EXERCISE_ST;
                        TRIGGER_VOID_EVENT; 
                    break;
                    }                     
                    default:
                        break;
                }
            break;
            }
            case UI_AO_MOVE_BAR_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        if(routine_to_do.ejercicios[selected_exercise].type_of_exercise == 2){
                            static MOTORS_AO_MOVE_PL horizontal_movement_event2 = {MOTORS_AO_MOVE_SIG, M2, -900};
                            Active_post(AO_Motors, (Event*)&horizontal_movement_event2);
                            display_rows("    Positioning     ", "         bar        ", "    horizontally    ", "                    ");  
                        }
                        else{
                            static MOTORS_AO_MOVE_PL vertical_movement_event2 = {MOTORS_AO_MOVE_SIG, M2, 0};
                            Active_post(AO_Motors, (Event*)&vertical_movement_event2);
                            display_rows("    Positioning     ", "         bar        ", "     vertically     ", "                    ");
                        }
                        inicio = true;
                    break;  
                    }
                    case UI_AO_ACK_MOVE_SIG:{
                        this->state = UI_AO_MOVE_TO_MIN_ANGLE_ST;
                        TRIGGER_VOID_EVENT; 
                    break;
                    } 
                    default:
                        break;
                }
            break;
            }    
            case UI_AO_MOVE_TO_MIN_ANGLE_ST:{
                switch ((e->sig)){
                    case UI_AO_TIMEOUT_SIG:{                        
                        if(inicio){    
                            degrees_to_send = routine_to_do.ejercicios[selected_exercise].lim_min;
                            printf("Degrees to send min: %d\n", degrees_to_send);                        
                            change_string(modified_buffer, 0, "Ex. ");
                            sprintf(char_data,"%ld", selected_exercise + 1);
                            change_string(modified_buffer, 4, char_data);
                            change_string(modified_buffer, 7, availableExercises[routine_to_do.ejercicios[selected_exercise].type_of_exercise]);
                            display_row1(modified_buffer);
                            display_row4("                    ");
                            if(routine_to_do.ejercicios[selected_exercise].type_of_exercise == 0){
                                
                                static MOTORS_AO_MOVE_PL min_angle_M2_movement_event = {MOTORS_AO_MOVE_SIG, M2};
                                min_angle_M2_movement_event.degrees = degrees_to_send;
                                Active_post(AO_Motors, (Event*)&min_angle_M2_movement_event);
                                display_row2("Motor aro, min angle");

                            }
                            else{
                                static MOTORS_AO_MOVE_PL min_angle_M1_movement_event = {MOTORS_AO_MOVE_SIG, M1};
                                min_angle_M1_movement_event.degrees = degrees_to_send;
                                Active_post(AO_Motors, (Event*)&min_angle_M1_movement_event);
                                display_row2("Motor base min angle");

                            }
                            change_string(modified_buffer, 0, "Current rep.: ");
                            sprintf(char_data,"%ld", current_repetition + 1);
                            change_string(modified_buffer, 14, char_data);
                            display_row3(modified_buffer);  
                            inicio = false;
                            counter = routine_to_do.ejercicios[selected_exercise].time_pos;                            
                        }
                        else{
                            if(counter>0){
                                change_string(modified_buffer, 0, "Hold ");
                                sprintf(char_data,"%ld", counter);
                                change_string(modified_buffer, 5, char_data);
                                display_row4(modified_buffer);
                                counter--;
                                TimeEvent_arm(&this->te, (1000/ portTICK_RATE_MS), 0U);
                            }
                            else{
                                this->state = UI_AO_MOVE_TO_MAX_ANGLE_ST;
                                inicio = true;
                                display_row4("                    ");
                                TRIGGER_VOID_EVENT;
                            }
                        }
                        
                    break;  
                    }
                    case UI_AO_ACK_MOVE_SIG:{
                        printf("ACK MIN POS\n");
                        TRIGGER_VOID_EVENT; 
                    break;
                    } 
                    default:
                        break;
                }            
            break;
            }
            case UI_AO_MOVE_TO_MAX_ANGLE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{                        
                        if(inicio){
                            degrees_to_send = routine_to_do.ejercicios[selected_exercise].lim_max;
                            printf("Degrees to send max: %d\n", degrees_to_send);
                            if(routine_to_do.ejercicios[selected_exercise].type_of_exercise == 0){
                                static MOTORS_AO_MOVE_PL max_angle_M2_movement_event = {MOTORS_AO_MOVE_SIG,M2};
                                max_angle_M2_movement_event.degrees = degrees_to_send;
                                Active_post(AO_Motors, (Event*)&max_angle_M2_movement_event);
                                display_row2("Motor aro, max angle");                                
                            }
                            else{
                                static MOTORS_AO_MOVE_PL max_angle_M1_movement_event = {MOTORS_AO_MOVE_SIG, M1};
                                max_angle_M1_movement_event.degrees = degrees_to_send;
                                Active_post(AO_Motors, (Event*)&max_angle_M1_movement_event);                                
                                display_row2("Motor base max angle");

                            } 
                            inicio = false;
                            counter = routine_to_do.ejercicios[selected_exercise].time_pos;
                        }
                        else{
                            if(counter>0){
                                change_string(modified_buffer, 0, "Hold ");
                                sprintf(char_data,"%ld", counter);
                                change_string(modified_buffer, 5, char_data);
                                display_row4(modified_buffer);
                                counter--;
                                TimeEvent_arm(&this->te, (1000/ portTICK_RATE_MS), 0U);
                            }
                            else{
                                this->state = UI_AO_END_OF_REPETITION_ST;
                                inicio = true;
                                TRIGGER_VOID_EVENT;
                            }
                        }
                        
                    break;  
                    }
                    case UI_AO_ACK_MOVE_SIG:{
                        printf("ACK MAX POS\n");
                        TRIGGER_VOID_EVENT; 
                    break;
                    } 
                    default:
                        break;
                }

            break;
            }
            case UI_AO_END_OF_REPETITION_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        if(current_repetition + 1 < routine_to_do.ejercicios[selected_exercise].num_of_reps){
                            current_repetition++;
                            this->state = UI_AO_MOVE_TO_MIN_ANGLE_ST;
                            TRIGGER_VOID_EVENT;
                        }
                        else if(selected_exercise + 1 < routine_to_do.num_ejercicios){
                            selected_exercise ++;
                            current_repetition = 0;
                            counter = routine_to_do.pause;
                            this->state = UI_AO_CENTER_DEVICE_ST;
                            TRIGGER_VOID_EVENT;
                            
                        }
                        else{
                            display_rows("   End of routine   ", "                    ", "   Well done!  :D   ", "    See you soon    ");
                            this->state = UI_AO_INICIO_ST;
                            TimeEvent_arm(&this->te, (1000/ portTICK_RATE_MS), 0U);

                        }
                    break;
                    }
                    default:
                        break;
                }
            break;
            }
            case UI_AO_END_OF_EXERCISE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        display_rows("    Pause before    ", "   next  exercise   ", "                    ", "                    ");
                        if(counter>0){
                            change_string(modified_buffer, 0, "Beginning in ");
                            sprintf(char_data,"%ld", counter);
                            change_string(modified_buffer, 13, char_data);
                            display_row4(modified_buffer);
                            counter--;
                            TimeEvent_arm(&this->te, (1000/ portTICK_RATE_MS), 0U);
                        }
                        else{
                            this->state = UI_AO_MOVE_BAR_ST;                                
                            TRIGGER_VOID_EVENT;
                        }
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


void display_rows(char text1[20], char text2[20], char text3[20], char text4[20]){
    display_row1(text1);
    display_row2(text2);
    display_row3(text3);
    display_row4(text4);
}

void display_row1(char text[20]){
    static PRINTER_AO_TEXT_PL print_sw1_event = {PRINTER_AO_TEXT0_SIG};
    sprintf(print_sw1_event.string_buffer, "%.20s",text);
    Active_post(AO_printer, (Event*)&print_sw1_event);
}
void display_row2(char text[20]){
    static PRINTER_AO_TEXT_PL print_sw2_event = {PRINTER_AO_TEXT1_SIG};
    sprintf(print_sw2_event.string_buffer, "%.20s",text);
    Active_post(AO_printer, (Event*)&print_sw2_event);
}
void display_row3(char text[20]){
    static PRINTER_AO_TEXT_PL print_sw3_event = {PRINTER_AO_TEXT2_SIG};
    sprintf(print_sw3_event.string_buffer, "%.20s",text);
    Active_post(AO_printer, (Event*)&print_sw3_event);
}
void display_row4(char text[20]){
    static PRINTER_AO_TEXT_PL print_sw4_event = {PRINTER_AO_TEXT3_SIG};
    sprintf(print_sw4_event.string_buffer, "%.20s",text);
    Active_post(AO_printer, (Event*)&print_sw4_event);
}

void change_string(char base[], int l, char addition[]){
    int k = strlen(addition);
    for(int i = l; i < l+k; i++){
        base[i] = addition[i-l];
    }
    for(int i = l+k; i < 20; i++){
        base[i] = ' ';
    }
}

void display_inicio(UI_State estado){
    display_row1(estado.title);
    display_row2("--------------------");
    select_option(estado.options[i]);
    display_row3(modified_buffer);
    display_row4(estado.options[i+1]);
}


void display_plus_button(UI_State estado){    
    if(i <  estado.num_of_options-1){
        select_option(estado.options[i+1]);
        display_row3(estado.options[i]);
        display_row4(modified_buffer);
        i++;        
    }  
}
void display_minus_button(UI_State estado){    
    if(i > 0){
        select_option(estado.options[i-1]);
        display_row3(modified_buffer);
        display_row4(estado.options[i]);
        i--;        
    }  
}                

void select_option(char option[20]){
    memcpy(modified_buffer, option, 20);
    modified_buffer[0] = '*';
}










