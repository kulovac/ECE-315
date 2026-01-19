/******************************************************************************
 * University    : University of Alberta
 * Course        : ECE 315 – Computer Interfacing
 * Project       : Tutorial Lab 0 – FreeRTOS Basics
 * File          : lab0_part2.c
 * Authors       : Antonio Alejandro Andara Lara
 * Last Modified : 17-Dec-2025
 *
 * Description :
 * This file demonstrates basic FreeRTOS functionality on a Xilinx platform.
 * A transmit task prints a message periodically while a one-shot software
 * timer verifies correct task execution after a fixed delay.
 *
 * Notes :
 * - 1 tab == 4 spaces
 * - This file is a modified instructional example for laboratory use.
 ******************************************************************************/

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Xilinx includes */
#include "xil_printf.h"
#include "xparameters.h"

/* Constants */
#define TIMER_ID                1
#define DELAY_5_SECONDS         5000UL
#define DELAY_1_SECOND          1000UL
#define TIMER_CHECK_THRESHOLD   5

/* Function prototypes */
static void prvTxTask(void *pvParameters);
static void vTimerCallback(TimerHandle_t pxTimer);

/* Task and timer handles */
static TaskHandle_t xTxTask = NULL;
static TimerHandle_t xTimer = NULL;

/* Global variables */
static char Message[25] = "\nWelcome to ECE 315 lab.\n";
static long TxtaskCntr = 0;

/*-----------------------------------------------------------*/

int main(void)
{
    const TickType_t x5seconds = pdMS_TO_TICKS(DELAY_5_SECONDS);

    xil_printf("*** Tutorial App started ***\r\n");

    /* Create the transmit task */
    xTaskCreate(
        prvTxTask,                     /* Task function */
        "Tx",                          /* Task name */
        configMINIMAL_STACK_SIZE,      /* Stack size */
        NULL,                          /* Parameters */
        tskIDLE_PRIORITY,              /* Priority */
        &xTxTask                       /* Task handle */
    );

    /* Create a one-shot software timer */
    xTimer = xTimerCreate(
        "Timer",					   /* Timer name */
        x5seconds,					   /* Delay time */
        pdFALSE,                       /* One-shot timer */
        (void *)TIMER_ID,			   /* timer ID */
        vTimerCallback				   /* Callback Function */
    );

    /* Verify timer creation */
    configASSERT(xTimer);

    /* Start the timer immediately */
    xTimerStart(xTimer, 0);

    /* Start the scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;);
}

/*-----------------------------------------------------------*/

static void prvTxTask(void *pvParameters)
{
    const TickType_t x1second = pdMS_TO_TICKS(DELAY_1_SECOND);

    for (;;)
    {
        xil_printf("%s", Message);
        TxtaskCntr++;

        /* Delay for 1 second */
        vTaskDelay(x1second);
    }
}

/*-----------------------------------------------------------*/

static void vTimerCallback(TimerHandle_t pxTimer)
{
    long lTimerId;

    configASSERT(pxTimer);

    lTimerId = (long)pvTimerGetTimerID(pxTimer);

    if (lTimerId != TIMER_ID)
    {
        xil_printf("\nFreeRTOS Hello World Example FAILED\n");
    }

    /*
     * The Tx task prints a message every 1 second.
     * This timer expires after 5 seconds and checks
     * whether the task ran the expected number of times.
     */
    if (TxtaskCntr >= TIMER_CHECK_THRESHOLD)
    {
        xil_printf("\nTxtaskCntr = %d\n", TxtaskCntr);
        xil_printf("\nFreeRTOS Hello World Example PASSED\n");
    }
    else
    {
        xil_printf("\nFreeRTOS Hello World Example FAILED\n");
    }

    /* Delete the transmit task */
    vTaskDelete(xTxTask);
}
