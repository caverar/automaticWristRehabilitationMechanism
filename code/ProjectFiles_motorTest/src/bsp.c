#include "bsp.h"

// Standard C libraries
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// SDK Libraries
#include "pico/stdlib.h"
#include "hardware/adc.h"
//#include "hardware/uart.h"
//#include "hardware/gpio.h"


// FreeAct
#include <FreeAct.h>
// Active Objects
#include "blinky_AO.h"
#include "Motors_AO.h"

// External AO calls
extern Active *AO_printer;
extern Active *AO_blinkyButton;
extern Active *AO_UI;


void BSP_init(void){

    // GPIO 15
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    
    // ADC buttons 
    adc_init();
    adc_set_clkdiv(4799);  //10Ks/s
    // Initialize ADC pin
    adc_gpio_init(28);
    adc_select_input(2);
    adc_run(true);
    
    buttons_past_states = 0x00;
    buttons_states = 0x00;
    buttons_levels[0] = 0;
    buttons_levels[1] = 0;
    buttons_levels[2] = 0;
    buttons_levels[3] = 0;
    buttons_levels[4] = 0;
}


bool sw_debounce(bool current_state, u_int16_t* level){

    if(current_state){  // load and discharge of software capacitor 
        *level = (*level >= DEBOUNCE_HIGH_LEVEL) ? DEBOUNCE_HIGH_LEVEL : *level+1;
    }else{
        *level = 0;
    }
    return *level >= DEBOUNCE_HIGH_LEVEL;
}

void debounce_all(uint16_t adc_val,
                  uint16_t* buttons_levels,
                  uint8_t* buttons_states){ 
    
    if(SW1_MIN_VAL<adc_val && adc_val<SW1_MAX_VAL){
        *buttons_states &= ~(0x01<<0);
        *buttons_states |= (0x01 && sw_debounce(true, &buttons_levels[0]))<<0;

    }else if(SW2_MIN_VAL<adc_val && adc_val<SW2_MAX_VAL){
        *buttons_states &= ~(0x01<<1);
        *buttons_states |= (0x01 && sw_debounce(true, &buttons_levels[1]))<<1;

    }else if(SW3_MIN_VAL<adc_val && adc_val<SW3_MAX_VAL){
        *buttons_states &= ~(0x01<<2);
        *buttons_states |= (0x01 && sw_debounce(true, &buttons_levels[2]))<<2;

    }else if(SW4_MIN_VAL<adc_val && adc_val<SW4_MAX_VAL){
        *buttons_states &= ~(0x01<<3);
        *buttons_states |= (0x01 && sw_debounce(true, &buttons_levels[3]))<<3;

    }else if(SW5_MIN_VAL<adc_val && adc_val<SW5_MAX_VAL){
        *buttons_states &= ~(0x01<<4);
        *buttons_states |= (0x01 && sw_debounce(true, &buttons_levels[4]))<<4;

    }else{
        *buttons_states = 0x00;
        *buttons_states |= (0x01 && sw_debounce(false, &buttons_levels[0]))<<0;
        *buttons_states |= (0x01 && sw_debounce(false, &buttons_levels[1]))<<1;
        *buttons_states |= (0x01 && sw_debounce(false, &buttons_levels[2]))<<2;
        *buttons_states |= (0x01 && sw_debounce(false, &buttons_levels[3]))<<3;
        *buttons_states |= (0x01 && sw_debounce(false, &buttons_levels[4]))<<4;
    }
}

/* Hooks ===================================================================*/
/* Application hooks used in this project ==================================*/
/* NOTE: only the "FromISR" API variants are allowed in vApplicationTickHook*/
void vApplicationTickHook(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;



    /* perform clock tick processing */
    TimeEvent_tickFromISR(&xHigherPriorityTaskWoken);

    
    debounce_all(adc_hw->result, buttons_levels, &buttons_states);



    switch(buttons_states & ~buttons_past_states){
        case 0x01:{

            static Event const sw1PressedEvt_2 = {BLINKY_AO_SW1_PRESSED_SIG};
            Active_postFromISR(AO_blinkyButton, &sw1PressedEvt_2,
                               &xHigherPriorityTaskWoken);
            break;
        }
        case 0x02:{

            static Event const sw2PressedEvt_2 = {BLINKY_AO_SW2_PRESSED_SIG};
            Active_postFromISR(AO_blinkyButton, &sw2PressedEvt_2,
                               &xHigherPriorityTaskWoken);
            break;
        }
        case 0x04:{

            static Event const sw3PressedEvt_2 = {BLINKY_AO_SW3_PRESSED_SIG};
            Active_postFromISR(AO_blinkyButton, &sw3PressedEvt_2,
                               &xHigherPriorityTaskWoken);
            break;
        }
        case 0x08:{

            static Event const sw4PressedEvt_2 = {BLINKY_AO_SW4_PRESSED_SIG};
            Active_postFromISR(AO_blinkyButton, &sw4PressedEvt_2,
                               &xHigherPriorityTaskWoken);
            break;
        }
        case 0x10:{

            static Event const sw5PressedEvt_2 = {BLINKY_AO_SW5_PRESSED_SIG};
            Active_postFromISR(AO_blinkyButton, &sw5PressedEvt_2,
                               &xHigherPriorityTaskWoken);
            break;
        }
        default:
            break;
    }

    buttons_past_states = buttons_states;





    /* notify FreeRTOS to perform context switch from ISR, if needed */
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
/*..........................................................................*/
void vApplicationIdleHook(void) {
#ifdef NDEBUG
    /* Put the CPU and peripherals to the low-power mode.
    * you might need to customize the clock management for your application,
    * see the datasheet for your particular Cortex-M3 MCU.
    */
    __WFI(); /* Wait-For-Interrupt */
#endif
}
/*..........................................................................*/
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    /* ERROR!!! */
}


/*..........................................................................*/
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must
* provide an implementation of vApplicationGetIdleTaskMemory() to provide
* the memory that is used by the Idle task.
*/
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside
    * this function then they must be declared static - otherwise they will
    * be allocated on the stack and so not exists after this function exits.
    */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the
    * Idle task's state will be stored.
    */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    * Note that, as the array is necessarily of type StackType_t,
    * configMINIMAL_STACK_SIZE is specified in words, not bytes.
    */
    *pulIdleTaskStackSize = sizeof(uxIdleTaskStack)/sizeof(uxIdleTaskStack[0]);
}


/*..........................................................................*/
/* error-handling function called by exception handlers in the startup code */
void Q_onAssert(char const *module, int loc) {
    /* NOTE: add here your application-specific error handling */
    (void)module;
    (void)loc;
#ifndef NDEBUG /* debug build? */
    /* light-up all LEDs */
    //GPIOF_AHB->DATA_Bits[LED_RED | LED_GREEN | LED_BLUE] = 0xFFU;

    /* tie the CPU in this endless loop and wait for the debugger... */
    while (1) {
    }
#else /* production build */
    /* TODO: do whatever is necessary to put the system in a fail-safe state */
    /* important!!! */
    NVIC_SystemReset(); /* reset the CPU */
#endif
}