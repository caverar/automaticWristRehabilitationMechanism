#include "bsp.h"

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
// Active Objects
#include "blinky_AO.h"
#include "printer_AO.h"
// External AO calls
extern Active *AO_printer;
extern Active *AO_blinkyButton;


void BSP_init(void){
    // GPIO 15

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);


}


/* Hooks ===================================================================*/
/* Application hooks used in this project ==================================*/
/* NOTE: only the "FromISR" API variants are allowed in vApplicationTickHook*/
void vApplicationTickHook(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // /* state of the button debouncing, see below */
    // static struct ButtonsDebouncing {
    //     uint32_t depressed;
    //     uint32_t previous;
    // } buttons = { 0U, 0U };
    // uint32_t current;
    // uint32_t tmp;

    /* perform clock tick processing */
    TimeEvent_tickFromISR(&xHigherPriorityTaskWoken);

    // /* Perform the debouncing of buttons. The algorithm for debouncing
    // * adapted from the book "Embedded Systems Dictionary" by Jack Ganssle
    // * and Michael Barr, page 71.
    // */
    // current = ~GPIOF_AHB->DATA_Bits[BTN_SW1]; /* read SW1 */
    // tmp = buttons.depressed; /* save the debounced depressed buttons */
    // buttons.depressed |= (buttons.previous & current); /* set depressed */
    // buttons.depressed &= (buttons.previous | current); /* clear released */
    // buttons.previous   = current; /* update the history */
    // tmp ^= buttons.depressed;     /* changed debounced depressed */

    // if ((tmp & BTN_SW1) != 0U) {  /* debounced SW1 state changed? */
    //     if ((buttons.depressed & BTN_SW1) != 0U) { /* is SW1 depressed? */
    //         /* post the "button-pressed" event from ISR */
    //         static Event const buttonPressedEvt = {BUTTON_PRESSED_SIG};
    //         Active_postFromISR(AO_blinkyButton, &buttonPressedEvt,
    //                            &xHigherPriorityTaskWoken);
    //     }
    //     else { /* the button is released */
    //          /* post the "button-released" event from ISR */
    //          static Event const buttonReleasedEvt = {BUTTON_RELEASED_SIG};
    //          Active_postFromISR(AO_blinkyButton, &buttonReleasedEvt,
    //                              &xHigherPriorityTaskWoken);
    //     }
    // }

    if(gpio_get(BUTTON_PIN)){
        const Event button_pressed_event = {BLINKY_AO_BUTTON_PRESSED_SIG};
        Active_post(AO_blinkyButton, (const Event*)&button_pressed_event);
    }

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