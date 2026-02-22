/* Lab 1, Part 2 - Seven-Segment Display & Keypad
 *
 * ECE-315 WINTER 2025 - COMPUTER INTERFACING
 * Created on: February 5, 2021
 * Modified on: July 26, 2023
 * Modified on: January 20, 2025
 * Author(s):  Shyama Gandhi, Antonio Andara Lara
 *
 * Summary:
 * 1) Declare & initialize the 7-seg display (SSD).
 * 2) Use xDelay to alternate between two digits fast enough to prevent flicker.
 * 3) Output pressed keypad digits on both SSD digits: current_key on right, previous_key on left.
 * 4) Print status changes and experiment with xDelay to find minimum flicker-free frequency.
 *
 * Deliverables:
 * - Demonstrate correct display of current and previous keys with no flicker.
 * - Print to the SDK terminal every time that theh variable `status` changes.
 */


// Include FreeRTOS Libraries
#include <FreeRTOS.h>
#include <portmacro.h>
#include <projdefs.h>
#include <task.h>
#include <queue.h>

// Include xilinx Libraries
#include <xparameters.h>
#include <xgpio.h>
#include <xscugic.h>
#include <xil_exception.h>
#include <xil_printf.h>
#include <sleep.h>
#include <xil_cache.h>

// Other miscellaneous libraries
#include "pmodkypd.h"

// Part 2 headers
#include "rgb_led.h"

// Device ID declarations
#define KYPD_DEVICE_ID      XPAR_GPIO_KYPD_BASEADDR
/*************************** Enter your code here ****************************/
#define SSD_DEVICE_ID       XPAR_GPIO_SSD_BASEADDR
#define PSHBTN_DEVICE_ID    XPAR_GPIO_INPUTS_BASEADDR
/*****************************************************************************/

// keypad key table
#define DEFAULT_KEYTABLE    "0FED789C456B123A"

// channel (subject to change)
#define SSD_CHANNEL         1
#define PSHBTN_CHANNEL      1

// Declaring the devices
PmodKYPD    KYPDInst;

/*************************** Enter your code here ****************************/
// TODO: Declare the seven-segment display peripheral here.
XGpio       SSDInst;
XGpio       rgbLedInst;
XGpio       pbInst;
/*****************************************************************************/

enum PWM_Control {
    TURN_DOWN,
    TURN_UP,
    UNKNOWN,
};

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// Function prototypes
void InitializeKeypad();
static void vKeypadTask( void *pvParameters );
static void vRgbTask(void *pvParameters);
static void vButtonsTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);
u32 SSD_decode(u8 key_value, u8 cathode);

// Queue handles
QueueHandle_t keypad_to_ssd_handle;
QueueHandle_t pushbutton_to_led_handle;



int main(void)
{
    int status;

    // Initialize keypad
    InitializeKeypad();

/*************************** Enter your code here ****************************/
    // TODO: Initialize SSD and set the GPIO direction to output.
    XGpio_Initialize(&SSDInst, SSD_DEVICE_ID);
    XGpio_SetDataDirection(&SSDInst, SSD_CHANNEL, 0x0); // sets all pins as outputs

    // initialize LEDS and set GPIO direction to output
    XGpio_Initialize(&rgbLedInst, RGB_LED_BASEADDR);
    XGpio_SetDataDirection(&rgbLedInst, RGB_CHANNEL, 0x0);

    // initialize pushbutton GPIO
    XGpio_Initialize(&pbInst, PSHBTN_DEVICE_ID);
    XGpio_SetDataDirection(&pbInst, PSHBTN_CHANNEL, 0x1);

    // queue creation
    keypad_to_ssd_handle = xQueueCreate(1, sizeof(u8));
    pushbutton_to_led_handle = xQueueCreate(1, sizeof(u32));
/*****************************************************************************/

    xil_printf("Initialization Complete, System Ready!\n");

    xTaskCreate(vKeypadTask,                    /* The function that implements the task. */
                "main task",                /* Text name for the task, provided to assist debugging only. */
                configMINIMAL_STACK_SIZE,   /* The stack allocated to the task. */
                NULL,                       /* The task parameter is not used, so set to NULL. */
                tskIDLE_PRIORITY,           /* The task runs at the idle priority. */
                NULL);
    
    xTaskCreate(vRgbTask,
                "rgb task", 
                configMINIMAL_STACK_SIZE, 
                NULL, 
                tskIDLE_PRIORITY, 
                NULL);

    xTaskCreate(vButtonsTask,
            "button task", 
            configMINIMAL_STACK_SIZE, 
            NULL, 
            tskIDLE_PRIORITY, 
            NULL);

    xTaskCreate(vDisplayTask,
        "display task", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY, 
        NULL);

    vTaskStartScheduler();
    while(1);
    return 0;
}


static void vKeypadTask( void *pvParameters )
{
    u8 new_key;
    u16 keystate;
    XStatus status, previous_status = KYPD_NO_KEY;

    xil_printf("Pmod KYPD app started. Press any key on the Keypad.\r\n");
    while (1){
        // Capture state of the keypad
        keystate = KYPD_getKeyStates(&KYPDInst);

        // Determine which single key is pressed, if any
        // if a key is pressed, store the value of the new key in new_key
        status = KYPD_getKeyPressed(&KYPDInst, keystate, &new_key);

        // Print key detect if a new key is pressed or if status has changed
        if (status == KYPD_SINGLE_KEY && previous_status == KYPD_NO_KEY){
            xil_printf("Key Pressed: %c\r\n", (char) new_key);
            xQueueOverwrite(keypad_to_ssd_handle, &new_key); // put in queue
        } else if (status == KYPD_MULTI_KEY && status != previous_status){
            xil_printf("Error: Multiple keys pressed\r\n");
        }
        
        // display the value of `status` each time it changes
        if (previous_status != status) {
            xil_printf("Status changed to: %d\n", status); 
        }
        previous_status = status;

    }
}


void InitializeKeypad()
{
    KYPD_begin(&KYPDInst, KYPD_DEVICE_ID);
    KYPD_loadKeyTable(&KYPDInst, (u8*) DEFAULT_KEYTABLE);
}

// This function is hard coded to translate key value codes to their binary representation
u32 SSD_decode(u8 key_value, u8 cathode)
{
    u32 result;

    // key_value represents the code of the pressed key
    switch(key_value){ // Handles the coding of the 7-seg display
        case 48: result = 0b00111111; break; // 0
        case 49: result = 0b00110000; break; // 1
        case 50: result = 0b01011011; break; // 2
        case 51: result = 0b01111001; break; // 3
        case 52: result = 0b01110100; break; // 4
        case 53: result = 0b01101101; break; // 5
        case 54: result = 0b01101111; break; // 6
        case 55: result = 0b00111000; break; // 7
        case 56: result = 0b01111111; break; // 8
        case 57: result = 0b01111100; break; // 9
        case 65: result = 0b01111110; break; // A
        case 66: result = 0b01100111; break; // B
        case 67: result = 0b00001111; break; // C
        case 68: result = 0b01110011; break; // D
        case 69: result = 0b01001111; break; // E
        case 70: result = 0b01001110; break; // F
        default: result = 0b00000000; break; // default case - all seven segments are OFF
    }

    // cathode handles which display is active (left or right)
    // by setting the MSB to 1 or 0
    if(cathode==0){
            return result;
    } else {
            return result | 0b10000000;
    }
}

// Based on the button input, convert to a PWM
// opcode for the LED
enum PWM_Control LED_decode(u32 input) {
    switch (input) {
        case 1: return TURN_UP;
        case 8: return TURN_DOWN;
        default: return UNKNOWN;
    }
}

static void vRgbTask(void *pvParameters)
{
    const uint8_t color = RGB_CYAN;
    const TickType_t xPeriod = 20;
    const TickType_t xDelay = 5;
    TickType_t xTimeOn = xPeriod;
    TickType_t xTimeOff = 0;
    u32 input_value;
    enum PWM_Control ctrl;

    while (1) {
        if (pdTRUE == xQueueReceive(pushbutton_to_led_handle, &input_value, 0)) {
            ctrl = LED_decode(input_value);
            switch (ctrl) {
            case TURN_DOWN:
                xTimeOff = MIN(xPeriod, xTimeOff + 1);
                xTimeOn = xPeriod - xTimeOff;
                xil_printf("Time On: %d --- Time Off: %d\n", xTimeOn, xTimeOff);
                break;
            case TURN_UP:
                xTimeOn = MIN(xPeriod, xTimeOn + 1);
                xTimeOff = xPeriod - xTimeOn;
                xil_printf("Time On: %d --- Time Off: %d\n", xTimeOn, xTimeOff);
                break;
            case UNKNOWN:
              break;
            }
        }

        if (xTimeOn != 0) {
            // ensure it turns off fully :)
            XGpio_DiscreteWrite(&rgbLedInst, RGB_CHANNEL, color);
            vTaskDelay(xTimeOn);
        }
        XGpio_DiscreteWrite(&rgbLedInst, RGB_CHANNEL, 0);
        vTaskDelay(xTimeOff);
    }
}

static void vButtonsTask(void *pvParameters) {
    const TickType_t xDelay = 100;
    u32 input_value;
    while (1) {
        input_value = XGpio_DiscreteRead(&pbInst, PSHBTN_CHANNEL);
        xQueueOverwrite(pushbutton_to_led_handle, &input_value);
        vTaskDelay(xDelay);
    }
}

static void vDisplayTask(void *pvParameters) {
    const TickType_t xDelay = 12; // portticks
    u8 new_key, current_key = 'x', previous_key = 'x';
    u32 ssd_value=0;

    while (1) {
        if (pdTRUE == xQueueReceive(keypad_to_ssd_handle, &new_key, 0)) {
            previous_key = current_key;
            current_key = new_key;
        }
        
        ssd_value = SSD_decode(current_key, (u8) 1); // right side, cat = 1
        XGpio_DiscreteWrite(&SSDInst, SSD_CHANNEL, ssd_value);
        vTaskDelay(xDelay);

        ssd_value = SSD_decode(previous_key, (u8) 0); // left side, cat = 0
        XGpio_DiscreteWrite(&SSDInst, SSD_CHANNEL, ssd_value);
        vTaskDelay(xDelay);
    }
}
