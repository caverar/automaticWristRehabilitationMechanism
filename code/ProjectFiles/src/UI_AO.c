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
static UI_State BEGIN_ROUTINE = {" Created routine    ", 4,{" Check Routine first", " Do routine now     ", " Set pause betw. ex.", " Set as default rout"}};
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
int8_t new_angle;

//counter
int8_t counter = 5;

//used to save numerical value in char value
char char_data[3];

//pause active
bool pause_active = false;

//Exercises for default routine:
Exercise default_exercise_1 = {0, 5, -45, 45, 5};
Exercise default_exercise_2 = {1, 5, -30, 30, 5};
Exercise default_exercise_3 = {2, 5, -20, 20, 5};

//Selected routine. 0: default routine, 1: created routine
uint selected_routine = 0;

//number of exercise to see or execute
uint selected_exercise = 0;

uint current_repetition = 0;
bool inicio;

//Routines
Routine created_routine;
Routine default_routine;
Routine routine_to_do;


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
    this->default_min_angle = -20;
    this->default_max_angle = 20;
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

    //How to do this? Memory
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
        // if(e->sig == ERROR_SIG){
        //     display_rows("    Error occured   ", "                    ", "   Restart device   ", "                    ");
        // }
        if(e->sig == UI_AO_SW5_PRESSED_SIG){
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
        switch(this->state){

            case UI_AO_PAUSE_ST:{
                display_rows("System was          ", "paused              ", "                    ", "                    ");              
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
                        this->state = UI_AO_INICIO_ST;//esta línea no iría
                        display_rows("                    ", CALIBRATE.title, "                    ", "                    ");
                        TimeEvent_arm(&this->te, (3000 / portTICK_RATE_MS), 0U);//esta tampoco
                        //enviar desde aquí una señal a control_AO para que genere el evento
                        break;
                    }
                    //esta señal es un ack que viene de control_AO después de terminar la calibración    
                    // case ACK:{
                    //     this->state = UI_AO_INICIO_ST;
                    //     TRIGGER_VOID_EVENT; 
                    //     break;
                    // }
                    
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
                        printf("\ne\n");
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
                        printf("\ne\n");
                        this->state = (i == 3) ? UI_AO_BEGIN_ROUTINE_ST : UI_AO_CONFIG_EXERCISE_ST;               
                        this->exercise_type = i;
                        i = 0;
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        printf("\nb\n");
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
                        printf("Current values:\n tipo: %d\nReps: %d\nmin ang.: %d\nmax ang.: %d\ntime: %d\nPause: %d\n",
                        this->exercise_type, this->reps, this->min_angle, this->max_angle, this->time_in_position, this->pause_between_exercises);                      
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
                        printf("\ne\n");
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
                        printf("\nb\n");
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
                        printf("\n+\n");
                        if(this->reps <this->max_reps){
                            this->reps++;
                            sprintf(char_data,"%ld", this->reps);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);                            
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        if(this->reps > 1){
                            this->reps--;
                            sprintf(char_data,"%ld", this->reps);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\ne\n");
                        sprintf(char_data,"%ld", this->reps);
                        change_string(CONFIG_EXERCISE.options[0], 14, char_data);
                        i = 0;
                        this->state = UI_AO_CONFIG_EXERCISE_ST; 
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        printf("\nb\n");
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
                            //envío de señal a control_AO para mover la barra vertical
                            display_rows("    Positioning     ", "         bar        ", "     vertically     ", "                    ");
                        }                        
                        else if(this->exercise_type == 2){
                            //envío de señal a control_AO para mover la barra horizontal
                            display_rows("    Positioning     ", "         bar        ", "    horizontally    ", "                    ");
                        }
                        this->state = UI_AO_MEASURE_ANGLE;// esto se va
                        TimeEvent_arm(&this->te, (2000 / portTICK_RATE_MS), 0U);//esto también se va
                    break;
                    }                    
                    //esta señal es un ack que viene de control_AO después de terminar el posicionamiento    
                    // case ACK:{
                    //     this->state = UI_AO_MEASURE_ANGLE;
                    //     TRIGGER_VOID_EVENT; 
                    //     break;
                    // }  
                    default:
                        break;
                }
            break;
            }
            case UI_AO_MEASURE_ANGLE:{             
                
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
                            printf("Envío de señal para medición de encoder disco\n");
                            // Enviar señal para que mida posición con encoder del disco.
                            // El evento que se genera envía cada ms el valor del encoder
                            // mediante la señal new measure
                        }
                        else{
                            printf("Envío de señal para medición de encoder base\n");
                            // Enviar señal para que mida posición con encoder de la base.
                            // El evento que se genera envía cada ms el valor del encoder
                            // mediante la señal new measure
                        }
                    break;
                    }                    
                    // case UI_AO_NEW_MEASURE_SIG:{                        
                    //     guardar ese valor que se recibe en new_angle:
                    //     sprintf(char_data,"%ld", new_angle);
                    //     change_string(modified_buffer, 0, char_data);
                    //     display_row4(modified_buffer);
                    // break;
                    // }
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\ne\n");
                        if(angle == 0){
                            new_angle = -56;//esto es sólo para probar, debe quitarse
                            this->min_angle = new_angle;
                            sprintf(char_data,"%ld", this->min_angle);
                            change_string(CONFIG_EXERCISE.options[1], 13, char_data);
                        }
                        else if(angle == 1){
                            new_angle = 90;//esto es sólo para probar, debe quitarse
                            this->max_angle = new_angle;
                            sprintf(char_data,"%ld", this->max_angle);
                            change_string(CONFIG_EXERCISE.options[2], 13, char_data);
                        }
                        i = 0;
                        this->state = UI_AO_CONFIG_EXERCISE_ST; 
                        TRIGGER_VOID_EVENT;                                       
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        printf("\nb\n");
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
                        printf("\n+\n");
                        if(this->time_in_position <this->max_secs){
                            this->time_in_position++;
                            sprintf(char_data,"%ld", this->time_in_position);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        if(this->time_in_position > 1){
                            this->time_in_position--;
                            sprintf(char_data,"%ld", this->time_in_position);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }                      
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\ne\n");
                        sprintf(char_data,"%ld", this->time_in_position);
                        change_string(CONFIG_EXERCISE.options[3], 7, char_data);
                        i = 0;
                        this->state = UI_AO_CONFIG_EXERCISE_ST; 
                        TRIGGER_VOID_EVENT;                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        printf("\nb\n");
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
                            case 3:{
                                this->state = UI_AO_SET_AS_DEFAULT_ST;
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
                        printf("Back\n");
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
                        printf("\n+\n");
                        if(this->pause_between_exercises <this->max_pause){
                            this->pause_between_exercises++;
                            sprintf(char_data,"%ld", this->pause_between_exercises);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        if(this->pause_between_exercises > 1){
                            this->pause_between_exercises--;
                            sprintf(char_data,"%ld", this->pause_between_exercises);
                            change_string(modified_buffer, 0, char_data);
                            display_row4(modified_buffer);
                        }                      
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\ne\n");
                        i = 0;
                        created_routine.pause = this->pause_between_exercises;
                        display_rows("   Pause  between   ", " exercises was set. ", "                    ", "                    ");
                        this->state = UI_AO_BEGIN_ROUTINE_ST; 
                        TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U);                                           
                    break;
                    }               
                    case UI_AO_SW1_PRESSED_SIG:{
                        printf("\nb\n");
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

            case UI_AO_SET_AS_DEFAULT_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{ 
                        printf("Previous created routine: \n");
                        for(int i = 0; i<created_routine.num_ejercicios; i++){
                                    printf("Exercise %d:\n", i);
                                    printf("tipo: %d\n",created_routine.ejercicios[i].type_of_exercise);
                                    printf("reps: %d\n",created_routine.ejercicios[i].num_of_reps);
                                    printf("min: %d\n",created_routine.ejercicios[i].lim_min);
                                    printf("max: %d\n",created_routine.ejercicios[i].lim_max);
                                    printf("secs: %d\n",created_routine.ejercicios[i].time_pos);
                        }
                        printf("Pause previous created routine: %d\n\n", created_routine.pause);
                        printf("Previous default routine: \n");
                        for(int i = 0; i< default_routine.num_ejercicios; i++){
                                    printf("Exercise %d:\n", i);
                                    printf("tipo: %d\n",default_routine.ejercicios[i].type_of_exercise);
                                    printf("reps: %d\n",default_routine.ejercicios[i].num_of_reps);
                                    printf("min: %d\n",default_routine.ejercicios[i].lim_min);
                                    printf("max: %d\n",default_routine.ejercicios[i].lim_max);
                                    printf("secs: %d\n",default_routine.ejercicios[i].time_pos);
                        }
                        printf("Pause previous default routine: %d\n\n", default_routine.pause);
                        default_routine = created_routine;
                        printf("----------------------\nAfter created routine: \n");
                        for(int i = 0; i<created_routine.num_ejercicios; i++){
                                    printf("Exercise %d:\n", i);
                                    printf("tipo: %d\n",created_routine.ejercicios[i].type_of_exercise);
                                    printf("reps: %d\n",created_routine.ejercicios[i].num_of_reps);
                                    printf("min: %d\n",created_routine.ejercicios[i].lim_min);
                                    printf("max: %d\n",created_routine.ejercicios[i].lim_max);
                                    printf("secs: %d\n",created_routine.ejercicios[i].time_pos);
                        }
                        printf("Pause after created routine: %d\n\n", created_routine.pause);
                        printf("After default routine: \n");
                        for(int i = 0; i< default_routine.num_ejercicios; i++){
                                    printf("Exercise %d:\n", i);
                                    printf("tipo: %d\n",default_routine.ejercicios[i].type_of_exercise);
                                    printf("reps: %d\n",default_routine.ejercicios[i].num_of_reps);
                                    printf("min: %d\n",default_routine.ejercicios[i].lim_min);
                                    printf("max: %d\n",default_routine.ejercicios[i].lim_max);
                                    printf("secs: %d\n",default_routine.ejercicios[i].time_pos);
                        }
                        printf("Pause after default routine: %d\n\n", default_routine.pause); 
                        display_rows("  Created  routine  ", "      was  set      ", " as default routine ", "                    ");                        
                        this->state = UI_AO_BEGIN_ROUTINE_ST;
                        TimeEvent_arm(&this->te, (2500 / portTICK_RATE_MS), 0U);   

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
                            default:
                                break;
                        }
                    i = 0; 
                    TRIGGER_VOID_EVENT;                                          
                    break;
                    }
                    case UI_AO_SW1_PRESSED_SIG:{
                        printf("Back\n");
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
                        printf("Enter\n");
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
                        printf("Back\n");
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
                        sprintf(char_data,"%ld", routine_to_do.ejercicios[selected_exercise].lim_min);
                        change_string(SHOW_AN_EXERCISE.options[2], 13, char_data);
                        sprintf(char_data,"%ld", routine_to_do.ejercicios[selected_exercise].lim_max);
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
                            this->state = UI_AO_CENTER_DEVICE_ST;
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
                        //envio de señal para generar evento a control_AO. Move motor de la base a 0
                        display_rows("      Centering     ", "       device       ", "                    ", "                    ");
                        this->state = UI_AO_MOVE_BAR_ST; // esto se va
                        TimeEvent_arm(&this->te, (2000 / portTICK_RATE_MS), 0U);//esto también se va
                    break;
                    }
                    //esta señal es un ack que viene de control_AO después de terminar de centrar  
                    // case ACK:{
                    //     this->state = UI_AO_MOVE_BAR_ST;
                    //     TRIGGER_VOID_EVENT; 
                    // break;
                    // }  

                    default:
                        break;
                }
            break;
            }
            case UI_AO_MOVE_BAR_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        if(routine_to_do.ejercicios[selected_exercise].type_of_exercise == 2){
                            //enviar señal a control_AO para mover barra horizontal
                            display_rows("    Positioning     ", "         bar        ", "    horizontally    ", "                    ");  
                        }
                        else{
                            //enviar señal a control_AO para mover barra vertical
                            display_rows("    Positioning     ", "         bar        ", "     vertically     ", "                    ");
                        }
                        this->state = UI_AO_MOVE_TO_MIN_ANGLE_ST; // esto se va
                        TimeEvent_arm(&this->te, (2000 / portTICK_RATE_MS), 0U);//esto también se va
                        inicio = true;
                    break;  
                    }
                    //esta señal es un ack que viene de control_AO después de terminar de posicionar la barra
                    // case ACK:{
                    //     this->state = UI_AO_MOVE_TO_MIN_ANGLE_ST;
                    //     TRIGGER_VOID_EVENT; 
                    // break;
                    // } 
                    default:
                        break;
                }
            break;
            }    
            case UI_AO_MOVE_TO_MIN_ANGLE_ST:{
                switch ((e->sig)){
                    case UI_AO_TIMEOUT_SIG:{
                        if(inicio){
                            change_string(modified_buffer, 0, "Ex. ");
                            sprintf(char_data,"%ld", selected_exercise + 1);
                            change_string(modified_buffer, 4, char_data);
                            change_string(modified_buffer, 7, availableExercises[routine_to_do.ejercicios[selected_exercise].type_of_exercise]);
                            display_row1(modified_buffer);
                            display_row4("                    ");
                            if(routine_to_do.ejercicios[selected_exercise].type_of_exercise == 0){
                                //envio de señal motor aro, min angle
                                display_row2("Motor aro, min angle");

                            }
                            else{
                                //envio de señal motor base, min angle
                                display_row2("Motor base min angle");

                            }
                            change_string(modified_buffer, 0, "Current rep.: ");
                            sprintf(char_data,"%ld", current_repetition + 1);
                            change_string(modified_buffer, 14, char_data);
                            display_row3(modified_buffer);  
                            inicio = false;
                            counter = routine_to_do.pause;
                            TimeEvent_arm(&this->te, (3000/ portTICK_RATE_MS), 0U);//esto también se va
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
                                TRIGGER_VOID_EVENT;
                            }
                        }
                        
                    break;  
                    }
                    //esta señal es un ack que viene de control_AO después de llegar al ángulo mínimo
                    // case ACK:{                       
                    //     TRIGGER_VOID_EVENT; 
                    // break;
                    // }
                    default:
                        break;
                }            
            break;
            }
            case UI_AO_MOVE_TO_MAX_ANGLE_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        if(inicio){
                            if(routine_to_do.ejercicios[selected_exercise].type_of_exercise == 0){
                                //envio de señal motor aro, max angle
                                display_row2("Motor aro, max angle");
                                display_row4("                    ");
                            }
                            else{
                                //envio de señal motor base, max angle
                                display_row2("Motor base max angle");

                            } 
                            inicio = false;
                            counter = routine_to_do.pause;
                            TimeEvent_arm(&this->te, (3000/ portTICK_RATE_MS), 0U);//esto también se va
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
                                TRIGGER_VOID_EVENT;
                            }
                        }
                        
                    break;  
                    }
                    //esta señal es un ack que viene de control_AO después de llegar al ángulo mínimo
                    // case ACK:{                       
                    //     TRIGGER_VOID_EVENT; 
                    // break;
                    // }
                    default:
                        break;
                }

            break;
            }
            case UI_AO_END_OF_REPETITION_ST:{
                switch(e->sig){
                    case UI_AO_TIMEOUT_SIG:{
                        display_row1("End of repetition   ");
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
//change plus and minus to modify just the last two rows   
    printf("boton +\n");    
    if(i <  estado.num_of_options-1){
        printf("Entramos a +");
        select_option(estado.options[i+1]);
        display_row3(estado.options[i]);
        display_row4(modified_buffer);
        i++;        
    } 
    printf("i is: %d\n", i);   
}
void display_minus_button(UI_State estado){
//change plus and minus to modify just the last two rows       
    printf("boton -\n");  
    if(i > 0){
        printf("Entramos a -");
        select_option(estado.options[i-1]);
        display_row3(modified_buffer);
        display_row4(estado.options[i]);
        i--;        
    }  
    printf("i is: %d\n", i);  
}                

void select_option(char option[20]){
    memcpy(modified_buffer, option, 20);
    modified_buffer[0] = '*';
    printf("Opcion seleccionada: %d\n%s\n", strlen(modified_buffer), modified_buffer);
}










