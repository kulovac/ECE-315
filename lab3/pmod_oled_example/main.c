// Include FreeRTOS Libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Include xilinx Libraries
#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_cache.h"

// Other miscellaneous libraries
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "pmodkypd.h"
#include "sleep.h"
#include "PmodOLED.h"
#include "OLEDControllerCustom.h"


#define BTN_DEVICE_ID  		XPAR_GPIO_INPUTS_BASEADDR
#define KYPD_DEVICE_ID 		XPAR_GPIO_KEYPAD_BASEADDR
#define KYPD_BASE_ADDR 		XPAR_GPIO_KEYPAD_BASEADDR
#define SSD_DEVICE_ID		XPAR_GPIO_SSD_BASEADDR
#define BTN_CHANNEL    1


#define FRAME_DELAY 50000

// keypad key table
#define DEFAULT_KEYTABLE 	"0FED789C456B123A"

// snake block size in pixels (4x4 square)
#define SNAKE_BLOCK_SIZE 4

// OLED screen sizes
#define OLED_LENGTH 32
#define OLED_WIDTH 128
#define NUM_X_CELLS OLED_WIDTH / SNAKE_BLOCK_SIZE
#define NUM_Y_CELLS OLED_LENGTH / SNAKE_BLOCK_SIZE

// Strings
// TODO: define numbers that will move
const char init_message[] = "Welcome to snake game!\nUse the keypad to move (<vals to move>)\nThe SSD shows your score\n";

// Declaring the devices
XGpio 		btnInst;
PmodOLED 	oledDevice;
PmodKYPD 	KYPDInst;
XGpio 		SSDInst;

// Function prototypes
void InitializeKeypad();
void initializeScreen();
static void keypadTask( void *pvParameters );
static void oledTask( void *pvParameters );
static void buttonTask( void *pvParameters );
static void ssdTask( void *pvParameters );
u32 SSD_decode(u8 key_value, u8 cathode);


const u8 orientation = 0x0; // Set up for Normal PmodOLED(false) vs normal
                            // Onboard OLED(true)
const u8 invert = 0x1; // true = whitebackground/black letters
                       // false = black background /white letters
u8 keypad_val = 'x';

typedef struct snake_block {
    struct snake_block *next; 
    
	u8 direction;
    u8 x;
    u8 y;
} snake_block;

// TODO: change these to reflect the values of desired keypad buttons
enum directions {
	UP = '2', 
	DOWN = '8', 
	LEFT = '6', 
	RIGHT = '4',
	NONE = 0
};

// Game values
u8 score = 0;
u8 current_direction = NONE;

int main()
{
	// ------------ Initialize Devices ------------
	InitializeKeypad();
	
	// initialize ssd
	XGpio_Initialize(&SSDInst, SSD_DEVICE_ID);
	XGpio_SetDataDirection(&SSDInst, SSD_CHANNEL, 0x0);

	// initialize oled
	// orientation: 0 is usually normal, invert: 0 = normal colors
    OLED_Begin(&oledDevice,
               XPAR_GPIO_OLED_BASEADDR,
               XPAR_SPI_OLED_BASEADDR,
               orientation,
               invert);

	// initialize buttons
	if(XGpio_Initialize(&btnInst, BTN_DEVICE_ID) != XST_SUCCESS){
		xil_printf("GPIO Initialization for SSD failed.\r\n");
		return XST_FAILURE;
	}


	xil_printf(init_message);

	// ------------ Create Tasks ------------
	xTaskCreate( keypadTask					/* The function that implements the task. */
			   , "keypad task"				/* Text name for the task, provided to assist debugging only. */
			   , configMINIMAL_STACK_SIZE	/* The stack allocated to the task. */
			   , NULL						/* The task parameter is not used, so set to NULL. */
			   , tskIDLE_PRIORITY			/* The task runs at the idle priority. */
			   , NULL
			   );


	xTaskCreate( oledTask					/* The function that implements the task. */
			   , "screen task"				/* Text name for the task, provided to assist debugging only. */
			   , configMINIMAL_STACK_SIZE	/* The stack allocated to the task. */
			   , NULL						/* The task parameter is not used, so set to NULL. */
			   , tskIDLE_PRIORITY			/* The task runs at the idle priority. */
			   , NULL
			   );

	xTaskCreate( buttonTask
			   , "button task"
			   , configMINIMAL_STACK_SIZE
			   , NULL
			   , tskIDLE_PRIORITY
			   , NULL
			   );

	xTaskCreate( ssdTask
			   , "ssd task"
			   , configMINIMAL_STACK_SIZE
			   , NULL
			   , tskIDLE_PRIORITY
			   , NULL
	);

	vTaskStartScheduler();

    while(1); // shouldn't get here, hang system

    return 0;
}


void InitializeKeypad()
{
   KYPD_begin(&KYPDInst, KYPD_BASE_ADDR);
   KYPD_loadKeyTable(&KYPDInst, (u8 *) DEFAULT_KEYTABLE);
}

// The controls
static void keypadTask( void *pvParameters ) {
	u16 keystate = 0;
	u16 last_keystate = 0; 
	XStatus status;
	XStatus last_status = KYPD_NO_KEY;
	u8 new_key = 0;
	const TickType_t xDelay = pdMS_TO_TICKS(10);

	while (1) {
		keystate = KYPD_getKeyStates(&KYPDInst);
		status = KYPD_getKeyPressed(&KYPDInst, keystate, &new_key);

		// detect if a new key is pressed (if status has changed)
		if (status == KYPD_SINGLE_KEY && last_status == KYPD_NO_KEY) {
			// update snake direction
			current_direction = new_key;
			xil_printf("Keypad button pressed: %x\r\n", new_key); // TODO: REMOVE
		} else if (status == KYPD_MULTI_KEY && last_status == KYPD_SINGLE_KEY) {
			// if new key and last keys pressed overlap
			if ((keystate & last_keystate) != 0) {
				// new keys take precedent
                KYPD_getKeyPressed(&KYPDInst, (keystate & ~last_keystate), &new_key);
                current_direction = new_key;

                xil_printf("Overlapping key pressed: %x\r\n", new_key); // TODO: REMOVE
			}
		}

		last_status = status;
        last_keystate = keystate;

		vTaskDelay(xDelay);
   }
}

// Actual displaying task
static void oledTask( void *pvParameters )
{
	u8 buttonVal = 0;
	char temp[10];

	OLED_SetDrawMode(&oledDevice, 0); // draw mode == set mode
	OLED_SetCharUpdate(&oledDevice, 0); // automatic updating off

	snake_block *head = start_game();

	while(1) {
		buttonVal = XGpio_DiscreteRead(&btnInst, BTN_CHANNEL);
		if (buttonVal == 0) {
			draw_snake(head);
			draw_consumable();
			update_game();
		} else if (buttonVal == 2) {
			OLED_ClearBuffer(&oledDevice);

			// show score on the OLED
			OLED_SetCursor(&oledDevice, 0, 0);
			sprintf(temp, "Score: %d", score);
			OLED_PutString(&oledDevice, temp);

			// show time on the OLED
			OLED_SetCursor(&oledDevice, 0, 2);
			u32 ticks = xTaskGetTickCount();
			ticks = ticks / 100;
			sprintf(temp, "Time: %lu", ticks);
			OLED_PutString(&oledDevice, temp);

			OLED_Update(&oledDevice);
		}
	}
}

// Shows menu options
static void buttonTask( void *pvParameters )
{
	const TickType_t xDelay = 10; // can be changed later

	u8 buttonVal = 0;
	while(1) {
		buttonVal = XGpio_DiscreteRead(&btnInst, BTN_CHANNEL);

		vTaskDelay(xDelay);
	}
}

// Displays points on the SSD
static void ssdTask(void *pvParameters) {
	const TickType_t xDelay = 10; // switch time (can be changed later)
	
	const u8 left_side = 0x1;
	const u8 right_side = 0x0; 

	u32 display_val = 0;
	u8 ones_digit = 0;
	u8 tens_digit = 0;

	while(1) {
		// display the ones digit of the score on the SSD
		ones_digit = score % 10;
		display_val = SSD_decode(ones_digit, right_side);
		XGpio_DiscreteWrite(&SSDInst, SSD_DEVICE_ID, display_val);
		
		vTaskDelay(xDelay);

		// display the tens digit of the score on the SSD
		tens_digit = (score / 10) % 10;
		display_val = SSD_decode(tens_digit, left_side);
		XGpio_DiscreteWrite(&SSDInst, SSD_DEVICE_ID, display_val);

		vTaskDelay(xDelay);
	}
}

u32 SSD_decode(u8 num, u8 cathode) {
    u32 result;
	
	// num == the value to display
	switch(num){ // Handles the coding of the 7-seg display
		case 0: result = 0b00111111; break; // 0
        case 1: result = 0b00110000; break; // 1
        case 2: result = 0b01011011; break; // 2
        case 3: result = 0b01111001; break; // 3
        case 4: result = 0b01110100; break; // 4
        case 5: result = 0b01101101; break; // 5
        case 6: result = 0b01101111; break; // 6
        case 7: result = 0b00111000; break; // 7
        case 8: result = 0b01111111; break; // 8
        case 9: result = 0b01111100; break; // 9
        default: result = 0b00000000; break; // default case - all seven segments are OFF
    }

	// cathode handles which display is active (left or right)
	// by setting the MSB to 1 or 0
    if(cathode == 0) {
        return result;
    } else {
        return result | 0b10000000;
	}
}

snake_block *start_game(void) {
	score = 0;
	snake_block *head = malloc(sizeof(snake_block));
	head->next = NULL;
	head->direction = NONE;
	head->x = 0;
	head->y = 0;
}

snake_block *create_snake_block(snake_block *next) {
	if (next == NULL) {
		xil_printf("nullptr error in create_snake_block\n");
		return;
	}
	snake_block *new_block = malloc(sizeof(snake_block));
	new_block->next = next;
	new_block->direction = next->direction;
	
	// TODO: add offsets
	new_block->x = 0;
	new_block->y = 0;
}

void free_snake_block(snake_block *block) {
	if (block->next == NULL) {
		free(block);
		return;
	}
	free_snake_block(block->next);
}

void draw_snake(snake_block *block) {
    const int half_size = SNAKE_BLOCK_SIZE / 2;

    // traverse the linked list
    while (block != NULL) {
		draw_block(block->x, block->y, half_size);
        block = block->next; 
    }
}

// TODO: maybe check the snake to avoid overlap
void draw_consumable(snake_block *block) {
	int x_pos = (rand() % NUM_X_CELLS) * SNAKE_BLOCK_SIZE;
	int y_pos = (rand() % NUM_Y_CELLS) * SNAKE_BLOCK_SIZE;
	const int half_size = SNAKE_BLOCK_SIZE / 2;

	draw_block(x_pos, y_pos, half_size);
}

void update_game(snake_block *block, snake_block *consumable) {
	
}

inline void draw_block(int x, int y, int half_size) {
	int rect_start  = x - half_size;
	int rect_end    = x + half_size;

	int rect_top    = y + half_size; 
	int rect_bottom = y - half_size;

	OLED_MoveTo(&oledDevice, rect_start, rect_top);
	OLED_RectangleTo(&oledDevice, rect_end, rect_bottom);
}