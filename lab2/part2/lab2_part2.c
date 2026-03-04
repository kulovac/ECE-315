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
 * 3) Output pressed keypad digits on both SSD digits: current_key on right,
 * previous_key on left. 4) Print status changes and experiment with xDelay to
 * find minimum flicker-free frequency.
 *
 * Deliverables:
 * - Demonstrate correct display of current and previous keys with no flicker.
 * - Print to the SDK terminal every time that theh variable `status` changes.
 */

#include <FreeRTOS.h>
#include <portmacro.h>
#include <projdefs.h>
#include <queue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <task.h>

#include <sleep.h>
#include <xgpio.h>
#include <xil_cache.h>
#include <xil_exception.h>
#include <xparameters.h>
#include <xscugic.h>

#include "pmodkypd.h"
#include "rgb_led.h"
#include "xuartps.h"

// Device ID declarations
#define KYPD_DEVICE_ID   XPAR_GPIO_KYPD_BASEADDR
#define SSD_DEVICE_ID    XPAR_GPIO_SSD_BASEADDR
#define PSHBTN_DEVICE_ID XPAR_GPIO_INPUTS_BASEADDR

// UART defs
#define UART_BASEADDR XPAR_UART1_BASEADDR
#define RX_QUEUE_LEN  512
#define CMD_QUEUE_LEN 16
#define TX_QUEUE_LEN  (512 * 2) // had to be increased to work

#define INPUT_TEXT_LEN 512

#define POLL_DELAY_MS 10 // changed from 1000

// keypad key table
#define DEFAULT_KEYTABLE "0FED789C456B123A"

// channel (subject to change)
#define SSD_CHANNEL    1
#define PSHBTN_CHANNEL 1

// Declaring the devices
PmodKYPD KYPDInst;
static XUartPs UartPs;
XGpio SSDInst;
XGpio rgbLedInst;
XGpio pbInst;

// Command enums
enum PWM_Control {
    TURN_DOWN,
    TURN_UP,
    UNKNOWN,
};

enum LED_SSD_Option {
    CMD_NONE,
    CMD_LED = '1',
    CMD_SSD = '2',
};

// Helpful macro functions
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

// Function prototypes
void InitializeKeypad(void);
static void vKeypadTask(void *pvParameters);
static void vRgbTask(void *pvParameters);
static void vButtonsTask(void *pvParameters);
static void vDisplayTask(void *pvParameters);
static void UART_RX_Task(void *pvParameters);
static void UART_TX_Task(void *pvParameters);
static void CLI_Task(void *pvParameters);
u32 SSD_decode(u8 key_value, u8 cathode);
enum PWM_Control LED_decode(u32 input);

// UART fns
uint8_t receive_byte(uint8_t *out_byte);
void receive_string(char *buf, size_t buf_len);
static void uart_init(void);
static int uart_poll_rx(uint8_t *b);
static void uart_tx_byte(uint8_t b);
void print_string(const char *str);
void flush_uart(void);

// Queue handles
QueueHandle_t keypad_to_ssd_handle;
QueueHandle_t pushbutton_to_led_handle;
QueueHandle_t rgb_cmd_handle;
QueueHandle_t rx_handle;
QueueHandle_t tx_handle;

// LED command format
typedef struct {
    uint8_t brightness; // 0-20
    uint8_t color;      // 0-7
} rgb_settings;

int main(void) {
    int status;

    // Initialize keypad and UART
    InitializeKeypad();
    uart_init();

    XGpio_Initialize(&SSDInst, SSD_DEVICE_ID);
    XGpio_SetDataDirection(&SSDInst, SSD_CHANNEL,
                           0x0); // sets all pins as outputs

    // initialize LEDS and set GPIO direction to output
    XGpio_Initialize(&rgbLedInst, RGB_LED_BASEADDR);
    XGpio_SetDataDirection(&rgbLedInst, RGB_CHANNEL, 0x0);

    // initialize pushbutton GPIO
    XGpio_Initialize(&pbInst, PSHBTN_DEVICE_ID);
    XGpio_SetDataDirection(&pbInst, PSHBTN_CHANNEL, 0x1);

    // queue creation
    keypad_to_ssd_handle     = xQueueCreate(2, sizeof(u8));
    pushbutton_to_led_handle = xQueueCreate(1, sizeof(u32));

    rgb_cmd_handle = xQueueCreate(1, sizeof(rgb_settings));
    rx_handle      = xQueueCreate(RX_QUEUE_LEN, sizeof(char));
    tx_handle      = xQueueCreate(TX_QUEUE_LEN, sizeof(char));
    /*****************************************************************************/

    print_string("Initialization Complete, System Ready!\n");

    xTaskCreate(vKeypadTask, /* The function that implements the task. */
                "main task", /* Text name for the task, provided to assist
                                debugging only. */
                configMINIMAL_STACK_SIZE, /* The stack allocated to the task. */
                NULL, /* The task parameter is not used, so set to NULL. */
                tskIDLE_PRIORITY, /* The task runs at the idle priority. */
                NULL);

    xTaskCreate(vRgbTask, "rgb task", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY, NULL);
    xTaskCreate(vButtonsTask, "button task", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "display task", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY, NULL);
    xTaskCreate(UART_RX_Task, "uart rx task", 1024, NULL, 3, NULL);
    xTaskCreate(UART_TX_Task, "uart tx task", 1024, NULL, 3, NULL);
    xTaskCreate(CLI_Task, "cli task", 1024, NULL, 3, NULL);

    vTaskStartScheduler();
    while (1)
        ;
    return 0;
}

static void UART_RX_Task(void *pvParameters) {
    (void)pvParameters;

    uint8_t byte;

    for (;;) {
        if (uart_poll_rx(&byte)) {
            xQueueSend(rx_handle, &byte, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
    }
}

static void UART_TX_Task(void *pvParameters) {
    (void)pvParameters;

    char c;

    for (;;) {
        if (xQueueReceive(tx_handle, &c, 0) == pdPASS) {
            uart_tx_byte((uint8_t)c);
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
    }
}

static void CLI_Task(void *pvParameters) {
    (void)pvParameters;

    char buf[INPUT_TEXT_LEN];
    uint8_t out_byte;
    char *color;
    rgb_settings rgb = {0};

    for (;;) {
        print_string("\nMenu:\n1. LED Command\n2. SSD Command\n");

        receive_byte(&out_byte);

        switch (out_byte) {
        case CMD_LED:
            print_string("\nEnter brightness (0 to 20) and color: ");
            receive_string(buf, INPUT_TEXT_LEN);

            rgb.brightness = atoi(strtok(buf, " "));
            color          = strtok(NULL, " ");

            if (strcmp(color, "red") == 0) {
                rgb.color = RGB_RED;
            } else if (strcmp(color, "green") == 0) {
                rgb.color = RGB_GREEN;
            } else if (strcmp(color, "blue") == 0) {
                rgb.color = RGB_BLUE;
            } else if (strcmp(color, "yellow") == 0) {
                rgb.color = RGB_YELLOW;
            } else if (strcmp(color, "cyan") == 0) {
                rgb.color = RGB_CYAN;
            } else if (strcmp(color, "magenta") == 0) {
                rgb.color = RGB_MAGENTA;
            } else if (strcmp(color, "white") == 0) {
                rgb.color = RGB_WHITE;
            } else {
                print_string("\nNot a valid color!\n");
                continue;
            }

            xQueueSend(rgb_cmd_handle, &rgb, 0);
            break;
        case CMD_SSD:
            print_string("\nEnter 2 hex digits to display: ");
            receive_string(buf, INPUT_TEXT_LEN);

            if (strlen(buf) != 2) {
                print_string("\nEnter 2 hex digits!!\n");
                continue;
            }

            xQueueSend(keypad_to_ssd_handle, buf, 0);
            xQueueSend(keypad_to_ssd_handle, buf + 1, 0);
            break;
        default: print_string("\nRTFM!!\n"); break;
        }
        vTaskDelay(1000);
    }
}

static void vKeypadTask(void *pvParameters) {
    (void)pvParameters;

    u8 new_key;
    u16 keystate;
    XStatus status, previous_status = KYPD_NO_KEY;
    const TickType_t xDelay = 50;

    while (1) {
        // Capture state of the keypad
        keystate = KYPD_getKeyStates(&KYPDInst);

        // Determine which single key is pressed, if any
        // if a key is pressed, store the value of the new key in new_key
        status = KYPD_getKeyPressed(&KYPDInst, keystate, &new_key);

        // Print key detect if a new key is pressed or if status has changed
        if (status == KYPD_SINGLE_KEY && previous_status == KYPD_NO_KEY) {
            xQueueSend(keypad_to_ssd_handle, &new_key, 0); // put in queue
        } else if (status == KYPD_MULTI_KEY && status != previous_status) {
            print_string("Error: Multiple keys pressed\r\n");
        }
        previous_status = status;
        vTaskDelay(xDelay);
    }
}

void InitializeKeypad(void) {
    KYPD_begin(&KYPDInst, KYPD_DEVICE_ID);
    KYPD_loadKeyTable(&KYPDInst, (u8 *)DEFAULT_KEYTABLE);
}

// This function is hard coded to translate key value codes to their binary
// representation
u32 SSD_decode(u8 key_value, u8 cathode) {
    u32 result;

    // key_value represents the code of the pressed key
    switch (key_value) { // Handles the coding of the 7-seg display
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
    default:
        result = 0b00000000;
        break; // default case - all seven segments are OFF
    }

    // cathode handles which display is active (left or right)
    // by setting the MSB to 1 or 0
    if (cathode == 0) {
        return result;
    } else {
        return result | 0b10000000;
    }
}

// Based on the button input, convert to a PWM
// opcode for the LED
enum PWM_Control LED_decode(u32 input) {
    switch (input) {
    case 1: return TURN_DOWN;
    case 8: return TURN_UP;
    default: return UNKNOWN;
    }
}

static void vRgbTask(void *pvParameters) {
    (void)pvParameters;

    uint8_t color            = RGB_CYAN;
    const TickType_t xPeriod = 20;
    TickType_t xTimeOn       = xPeriod;
    TickType_t xTimeOff      = 0;
    u32 input_value;
    enum PWM_Control ctrl;
    rgb_settings rgb_cmd;

    while (1) {

        if (pdPASS == xQueueReceive(rgb_cmd_handle, &rgb_cmd, 0)) {
            xTimeOn  = MAX(MIN(rgb_cmd.brightness, xPeriod), 0);
            xTimeOff = xPeriod - xTimeOn;
            color    = rgb_cmd.color;
        }

        if (pdPASS ==
            xQueueReceive(pushbutton_to_led_handle, &input_value, 0)) {
            ctrl = LED_decode(input_value);
            switch (ctrl) {
            case TURN_DOWN:
                xTimeOff = MIN(xPeriod, xTimeOff + 1);
                xTimeOn  = xPeriod - xTimeOff;
                break;
            case TURN_UP:
                xTimeOn  = MIN(xPeriod, xTimeOn + 1);
                xTimeOff = xPeriod - xTimeOn;
                break;
            case UNKNOWN: break;
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
    (void)pvParameters;

    const TickType_t xDelay = 50;
    u32 input_value;
    while (1) {
        input_value = XGpio_DiscreteRead(&pbInst, PSHBTN_CHANNEL);
        xQueueOverwrite(pushbutton_to_led_handle, &input_value);
        vTaskDelay(xDelay);
    }
}

static void vDisplayTask(void *pvParameters) {
    (void)pvParameters;

    const TickType_t xDelay = 12; // portticks
    u8 new_key, current_key = 'x', previous_key = 'x';
    u32 ssd_value = 0;

    while (1) {
        if (pdPASS == xQueueReceive(keypad_to_ssd_handle, &new_key, 0)) {
            previous_key = current_key;
            current_key  = new_key;
        }

        ssd_value = SSD_decode(current_key, (u8)1); // right side, cat = 1
        XGpio_DiscreteWrite(&SSDInst, SSD_CHANNEL, ssd_value);
        vTaskDelay(xDelay);

        ssd_value = SSD_decode(previous_key, (u8)0); // left side, cat = 0
        XGpio_DiscreteWrite(&SSDInst, SSD_CHANNEL, ssd_value);
        vTaskDelay(xDelay);
    }
}

static void uart_init(void) {
    XUartPs_Config *cfg;

    cfg = XUartPs_LookupConfig(UART_BASEADDR);
    if (!cfg) {
        while (1) {
        }
    }

    if (XUartPs_CfgInitialize(&UartPs, cfg, cfg->BaseAddress) != XST_SUCCESS) {
        while (1) {
        }
    }

    XUartPs_SetBaudRate(&UartPs, 115200);
}

uint8_t receive_byte(uint8_t *out_byte) {
    while (1) {
        if (xQueueReceive(rx_handle, out_byte, 0) != pdPASS) {
            vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
        } else {
            return *out_byte;
        }
    }
}

void receive_string(char *buf, size_t buf_len) {
    uint8_t recvd;
    size_t idx = 0;
    buf[0]     = '\0';

    while (1) {
        receive_byte(&recvd);

        if (idx == buf_len) {
            if (recvd == '\r') {
                buf[buf_len - 1] = '\0';
                break;
            } else {
                continue;
            }
        }

        if (recvd == '\r') {
            buf[idx] = '\0';
            break;
        }

        buf[idx++] = recvd;
        vTaskDelay(pdMS_TO_TICKS(POLL_DELAY_MS));
    }
}

void flush_uart(void) {
    uint8_t dummy;
    while (xQueueReceive(rx_handle, &dummy, 0) == pdPASS)
        ;
}

void print_string(const char *str) {
    if (str == NULL) return;
    for (; *str != '\0'; ++str) {
        xQueueSend(tx_handle, str, 0);
    }
}

static int uart_poll_rx(uint8_t *b) {
    if (XUartPs_IsReceiveData(UartPs.Config.BaseAddress)) {
        *b = XUartPs_ReadReg(UartPs.Config.BaseAddress, XUARTPS_FIFO_OFFSET);
        return 1;
    }
    return 0;
}

static void uart_tx_byte(uint8_t b) {
    while (XUartPs_IsTransmitFull(UartPs.Config.BaseAddress)) {
    }

    XUartPs_WriteReg(UartPs.Config.BaseAddress, XUARTPS_FIFO_OFFSET, b);
}
