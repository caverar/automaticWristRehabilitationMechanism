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
static UI_State INICIO = {" Choose an option:", 2,{" Create Routine", " Do default routine"}};
static UI_State CREATE = {" Add exercise", 4, {" PronoSupination", " FlexoExtension", " Ab-,Adduction", " Begin Routine"}};
static UI_State DO_DEFAULT = {" Do default routine"};
static UI_State CONFIG_EXERCISE = {"", 5, {" Repetitons: 1", " Min. Angle: -20", " Max. Angle: 20", " Time: 3", " Exercise Ready"}};
static UI_State REPETITIONS = {};
static UI_State MIN_ANGLE = {};
static UI_State MAX_ANGLE = {};
static UI_State TIME = {};
static UI_State EXERCISE_READY = {};
static UI_State BEGIN_ROUTINE = {" Begin routine", 4,{" Check Routine first", " Do routine now", " Set pause betw. ex.", " Set as default rout"}};
static UI_State CHECK_ROUTINE = {};
static UI_State DO_ROUTINE_NOW = {}; 
static UI_State SET_PAUSE = {};
static UI_State SET_AS_DEFAULT = {};

//Global iterator
uint i = 0;


//Global buffer for LCD screen
char modified_buffer[20];

char availableExercises[3][20] = {"PronoSup.", "FlexoExt.", "Ab-,Adduc."};

int8_t prev_reps;
int8_t prev_time;
char char_data[3];

//Exercises for default routine:
Exercise default_exercise_1 = {0, 5, -45, 45, 5};
Exercise default_exercise_2 = {1, 5, -30, 30, 5};
Exercise default_exercise_3 = {2, 5, -20, 20, 5};
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
        switch(this->state){

            case UI_AO_HOME_ST:{

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{

                        UI_show(HOME.title, "", "", "");
                        this->state = UI_AO_CALIBRATE_ST;
                        static PRINTER_AO_TEXT_PL print_sw1_event = {PRINTER_AO_TEXT1_SIG};
                        sprintf(print_sw1_event.string_buffer, HOME.title);
                        Active_post(AO_printer, (Event*)&print_sw1_event);
                        TimeEvent_arm(&this->te, (2000 / portTICK_RATE_MS), 0U);

                    break;
                    }default:
                        break;                
                }
            break;
            }
            case UI_AO_CALIBRATE_ST:{

                switch (e->sig){

                    case UI_AO_TIMEOUT_SIG:{
                        UI_show(CALIBRATE.title, "", "", "");
                        this->state = UI_AO_INICIO_ST;
                        // static PRINTER_AO_TEXT_PL print_sw1_event1 = {PRINTER_AO_TEXT1_SIG};
                        // sprintf(print_sw1_event1.string_buffer, CALIBRATE.title);
                        // Active_post(AO_printer, (Event*)&print_sw1_event1);
                        TimeEvent_arm(&this->te, (3000 / portTICK_RATE_MS), 0U);
                        break;
                    }default:
                        break;
                }
            break;
            }
            case UI_AO_INICIO_ST:{              
                switch (e->sig){               
                    case UI_AO_TIMEOUT_SIG:{
                        UI_show_inicio(INICIO);
                        //display title
                        // static PRINTER_AO_TEXT_PL print_sw1_event2 = {PRINTER_AO_TEXT0_SIG};
                        // sprintf(print_sw1_event2.string_buffer, INICIO.title);
                        // Active_post(AO_printer, (Event*)&print_sw1_event2);
                        //display ---
                        // static PRINTER_AO_TEXT_PL print_sw1_eventLine = {PRINTER_AO_TEXT1_SIG};
                        // sprintf(print_sw1_eventLine.string_buffer, "--------------------");
                        // Active_post(AO_printer, (Event*)&print_sw1_eventLine);                     
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        printf("\n+\n");
                        UI_show_plus_button(INICIO);
                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        UI_show_minus_button(INICIO);
                                           
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

                        UI_show_inicio(CREATE);

                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        printf("\n+\n");
                        UI_show_plus_button(CREATE);
                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        UI_show_minus_button(CREATE);
                                           
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
                        printf("Current values: tipo: %d\nReps: %d\nmin ang.: %d\nmax ang.: %d\ntime: %d\nPause: %d\n",
                        this->exercise_type, this->reps, this->min_angle, this->max_angle, this->time_in_position, this->pause_between_exercises);                      
                        strcpy( CONFIG_EXERCISE.title, "Config. ");
                        strcat( CONFIG_EXERCISE.title, availableExercises[this->exercise_type]);
                        UI_show_inicio(CONFIG_EXERCISE);                     
                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        printf("\n+\n");
                        UI_show_plus_button(CONFIG_EXERCISE);
                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        UI_show_minus_button(CONFIG_EXERCISE);
                                           
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
                                this->state = UI_AO_MIN_ANGLE_ST;
                            break;
                            }
                            case 2:{
                                this->state = UI_AO_MAX_ANGLE_ST;
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
                        UI_show(" Set number of ", " repetitions: ", "---", char_data);

                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        printf("\n+\n");
                        if(this->reps <this->max_reps){
                            this->reps++;
                            sprintf(char_data,"%ld", this->reps);
                            UI_show(" Set number of ", " repetitions: ", "---", char_data);
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        if(this->reps > 1){
                            this->reps--;
                            sprintf(char_data,"%ld", this->reps);
                            UI_show(" Set number of ", " repetitions: ", "---", char_data);
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\ne\n");
                        sprintf(char_data,"%ld", this->reps);
                        strcpy( CONFIG_EXERCISE.options[0], " Repetitions: ");
                        strcat( CONFIG_EXERCISE.options[0], char_data);
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

            case UI_AO_MIN_ANGLE_ST:{              

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{                        
                        UI_show("Move to min. angle ", "Then press enter   ", "---", "");
                        //Crear evento y postearlo al encoder para visualizar cambio del
                        //ángulo en la pantalla

                    break;
                    }
                    //cambiar esta por una señal que venga del encoder AO
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\nmin angle was set\n");                        
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
            case UI_AO_MAX_ANGLE_ST:{              

                switch (e->sig){
                    
                    case UI_AO_TIMEOUT_SIG:{                        
                        UI_show("Move to max. angle ", "Then press enter   ", "---", "");
                        //Crear evento y postearlo al encoder para visualizar cambio del
                        //ángulo en la pantalla

                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\nmax angle was set\n");                        
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
                        UI_show(" Set time in ", " seconds: ", "---", char_data);

                    break;
                    }
                    case UI_AO_SW3_PRESSED_SIG:{
                        printf("\n+\n");
                        if(this->time_in_position <this->max_secs){
                            this->time_in_position++;
                            sprintf(char_data,"%ld", this->time_in_position);
                            UI_show(" Set time in ", " seconds: ", "---", char_data);
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW2_PRESSED_SIG:{
                        printf("\n-\n");
                        if(this->time_in_position > 1){
                            this->time_in_position--;
                            sprintf(char_data,"%ld", this->time_in_position);
                            UI_show(" Set time in ", " seconds: ", "---", char_data);
                        }                       
                                           
                    break;
                    }
                    case UI_AO_SW4_PRESSED_SIG:{
                        printf("\ne\n");
                        sprintf(char_data,"%ld", this->time_in_position);
                        strcpy( CONFIG_EXERCISE.options[3], " Time: ");
                        strcat( CONFIG_EXERCISE.options[3], char_data);
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
            default:
                break;


        }
    }
}


//add function to show first row
//add function to set ------ in the second row
//add function to show title
void UI_show_inicio(UI_State estado){
    //modify to change the last two rows
    UI_selec(estado.options[i]);
    UI_show(estado.title, "--------------------", modified_buffer,
    estado.options[i+1]);
    printf("i is: %d", i);   
}

                      
void UI_show_plus_button(UI_State estado){
//change plus and minus to modify just the last two rows       
    if(i <  estado.num_of_options-1){
        UI_selec(estado.options[i+1]);
        UI_show(estado.title, "--------------------", estado.options[i],
        modified_buffer);
        i++;        
    } 
    printf("i is: %d", i);   
}


void UI_show_minus_button(UI_State estado){
    
    if(i > 0){
        UI_selec(estado.options[i-1]);
        UI_show(estado.title, "--------------------", modified_buffer,
        estado.options[i]);
        i--;        
    } 
    printf("i is: %d", i);   
}

void UI_show(char* row1, char* row2, char* row3, char* row4){
    printf("\n*******Pantalla*******\n");
    printf("***********************\n");
    printf("%s\n", row1);
    printf("%s\n", row2);
    printf("%s\n", row3);
    printf("%s\n", row4);
    printf("***********************\n");
}

void UI_selec(char* a){
    memcpy(modified_buffer, a, 20);
    modified_buffer[0] = '*';
    printf("Lennnn: %d: ", strlen(modified_buffer));
}




