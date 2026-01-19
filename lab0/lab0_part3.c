/******************************************************************************
 * University  : University of Alberta
 * Course      : ECE 315 – Computer Interfacing
 * Project     : Tutorial Lab 0 – AXI GPIO and FreeRTOS
 * File        : lab0_part3.c
 *
 * Original Authors :
 *      Shyama M. Gandhi
 *      Mazen Elbaz
 *
 * Modified By :
 *      Antonio Alejandro Andara Lara
 *
 * Original Date :
 *      July 07, 2021
 * Last Modified :
 *      17-Dec-2025
 *
 * Description :
 * This file demonstrates the use of AXI GPIO with FreeRTOS on the Zynq-7000
 * platform. Two FreeRTOS tasks control the on-board RGB LED and green LEDs
 * using GPIO operations and task scheduling with different priorities. *
 ******************************************************************************/

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Xilinx includes */
#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"

/* GPIO parameter definitions (from xparameters.h) */
#define LEDS_DEVICE_ID        XPAR_AXI_GPIO_LEDS_BASEADDR

/* LED channels */
#define GREEN_LEDS_CHANNEL   1
#define RGB_CHANNEL          2

/* RGB color definitions */
#define BLUE_IN_RGB          1
#define GREEN_IN_RGB         2
#define CYAN_IN_RGB          3
#define RED_IN_RGB           4
#define PINK_IN_RGB          5
#define YELLOW_IN_RGB        6
#define WHITE_IN_RGB         7

/* GPIO instances */
static XGpio LEDInst;
static XGpio RGBInst;

/* Task prototypes */
static void vBlueLedTask(void *pvParameters);
static void vYellowLedTask(void *pvParameters);

/* Task messages */
static const char *pcTextForTask1 = "Blue task is running";
static const char *pcTextForTask2 = "Yellow task is running";

/*-----------------------------------------------------------*/

int main(void)
{
    int status;

    /*----------------------------------------------------
     * Initialize GPIO peripherals and set directions
     *----------------------------------------------------*/

    /* Initialize green LEDs */
    status = XGpio_Initialize(&LEDInst, LEDS_DEVICE_ID);
    if (status != XST_SUCCESS)
    {
        xil_printf("GPIO Initialization for LEDs unsuccessful.\r\n");
        return XST_FAILURE;
    }

    /* Initialize RGB LED */
    status = XGpio_Initialize(&RGBInst, LEDS_DEVICE_ID);
    if (status != XST_SUCCESS)
    {
        xil_printf("GPIO Initialization for RGB LED unsuccessful.\r\n");
        return XST_FAILURE;
    }

    /*
     * GPIO direction:
     * 0 = output
     * 1 = input
     */
    XGpio_SetDataDirection(&LEDInst, GREEN_LEDS_CHANNEL, 0x00);
    XGpio_SetDataDirection(&RGBInst, RGB_CHANNEL, 0x00);

    xil_printf("Initialization done for RGB LED and LEDs ::: System Ready\r\n\n");

    /* Create FreeRTOS tasks */
    xTaskCreate(
        vBlueLedTask,
        "Blue Task",
        1000,
        (void *)pcTextForTask1,
        3,              /* Higher priority */
        NULL
    );

    xTaskCreate(
        vYellowLedTask,
        "Yellow Task",
        1000,
        (void *)pcTextForTask2,
        2,
        NULL
    );

    /* Start the scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;);

    return 0;
}

/*-----------------------------------------------------------*/

static void vBlueLedTask(void *pvParameters)
{
    const TickType_t xDelay1800ms = pdMS_TO_TICKS(1800UL);
    uint32_t current_state;
    uint32_t new_state;
    
    for (;;)
    {
        xil_printf("||| %s |||\n", (char *)pvParameters);

        /* Read current LED state */
        current_state = XGpio_DiscreteRead(&LEDInst, GREEN_LEDS_CHANNEL);

        /* Set LSB to 1 */
        new_state = current_state | 0b0001;

        /* Update LEDs */
        XGpio_DiscreteWrite(&LEDInst, GREEN_LEDS_CHANNEL, new_state);
        
        /* Set RGB LED to blue */
        XGpio_DiscreteWrite(&RGBInst, RGB_CHANNEL, BLUE_IN_RGB);
        
        vTaskDelay(xDelay1800ms);

        /* Clear LSB */
        current_state = XGpio_DiscreteRead(&LEDInst, GREEN_LEDS_CHANNEL);
        new_state = current_state & 0b1110;

        XGpio_DiscreteWrite(&LEDInst, GREEN_LEDS_CHANNEL, new_state);

        vTaskDelay(xDelay1800ms);
    }
}

/*-----------------------------------------------------------*/

static void vYellowLedTask(void *pvParameters)
{
    const TickType_t xDelay600ms = pdMS_TO_TICKS(600UL);
    const uint32_t yellow_mask = 0b0010;
    uint32_t led_state;

    for (;;)
    {
        xil_printf("|| %s ||\n", (char *)pvParameters);

        /* Set yellow LED bit */
        led_state = XGpio_DiscreteRead(&LEDInst, GREEN_LEDS_CHANNEL) | yellow_mask;
        XGpio_DiscreteWrite(&LEDInst, GREEN_LEDS_CHANNEL, led_state);

        /* Set RGB LED to yellow */
        XGpio_DiscreteWrite(&RGBInst, 2, YELLOW_IN_RGB);

        vTaskDelay(xDelay600ms);

        /* Clear yellow LED bit */
        led_state = XGpio_DiscreteRead(&LEDInst, GREEN_LEDS_CHANNEL) & ~yellow_mask;
        XGpio_DiscreteWrite(&LEDInst, GREEN_LEDS_CHANNEL, led_state);

        vTaskDelay(xDelay600ms);
    }
}

/******************************************************************************
 * ====================== CODE REVIEW QUESTIONS ======================
 *
 * This file intentionally uses different coding styles.
 *
 * 1. Identify all "magic numbers" used in this file.
 *    What do they represent?
 *
 * 2. Compare the bit manipulation approaches:
 *    - Explicit binary literals
 *    - Named bit masks
 *    Which is more readable or maintainable?
 *
 * 3. The two tasks use different priorities and delays.
 *    How does this affect execution?
 *
 * 4. Both tasks access the same GPIO peripheral.
 *    Why does this work here, and when would it be unsafe?
 *
 * ================================================================
 ******************************************************************************/
