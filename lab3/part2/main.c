// Include FreeRTOS Libraries
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Include xilinx Libraries
#include "xgpio.h"
#include "xil_printf.h"

// Other miscellaneous libraries
#include <projdefs.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <xstatus.h>
#include "pmodkypd.h"
#include "PmodOLED.h"
#include "OLEDControllerCustom.h"


#define BTN_DEVICE_ID       XPAR_GPIO_INPUTS_BASEADDR
#define KYPD_DEVICE_ID      XPAR_GPIO_KEYPAD_BASEADDR
#define KYPD_BASE_ADDR      XPAR_GPIO_KEYPAD_BASEADDR
#define SSD_DEVICE_ID       XPAR_GPIO_SSD_BASEADDR
#define BTN_CHANNEL         1
#define SSD_CHANNEL         1


#define FRAME_DELAY_MS      200
#define GAME_OVER_TIME_MS   2000

#define DIR_QUEUE_LEN       3
#define BTN_QUEUE_LEN       1
#define SCORE_QUEUE_LEN     2

// keypad key table
#define DEFAULT_KEYTABLE    "0FED789C456B123A"

// snake block size in pixels (4x4 square)
#define SNAKE_BLOCK_SIZE    4

// OLED screen sizes
#define OLED_LENGTH         32
#define OLED_WIDTH          128
#define NUM_X_CELLS         (OLED_WIDTH / SNAKE_BLOCK_SIZE)
#define NUM_Y_CELLS         (OLED_LENGTH / SNAKE_BLOCK_SIZE)

// Strings
const char init_message[] = 
" ------ Welcome to snake game! ------\n"
"Use the keypad to move (%c = UP, %c = DOWN, %c = LEFT, %c = RIGHT)\n"
"Use the pushbuttons for stats/reset (BTN1 = MENU, BTN2 = GAME OVER)\n"
"The SSD shows your score\n";

// Declaring the devices
XGpio       btnInst;
PmodOLED    oledDevice;
PmodKYPD    KYPDInst;
XGpio       SSDInst;

typedef struct snake_block {
    struct snake_block *next; 
    u8 x;
    u8 y;
} snake_block;


// Function prototypes
void InitializeKeypad();
void initializeScreen();
static void keypadTask( void *pvParameters );
static void oledTask( void *pvParameters );
static void buttonTask( void *pvParameters );
static void ssdTask( void *pvParameters );
static u32 SSD_decode(u8 key_value, u8 cathode);
static snake_block *start_game(void);
static snake_block *create_consumable(void);
static void draw_snake(snake_block *block);
static void draw_block(int x, int y);
static void move_snake(snake_block *block, u8 current_direction);
static void consume_point(snake_block *head, snake_block *consumable, u8 current_direction);
static int update_game(snake_block **head, snake_block **consumable, u8 current_direction);
static void game_over(snake_block **head, snake_block **consumable);


const u8 orientation = 0x1; // Set up for Normal PmodOLED(false) vs normal
                            // Onboard OLED(true)
const u8 invert = 0x0; // true = whitebackground/black letters
                       // false = black background /white letters
u8 keypad_val = 'x';

enum directions {
    UP = '2', 
    DOWN = '5', 
    LEFT = '4', 
    RIGHT = '6',
    NONE = 0
};

enum game_states {
    PLAY = 0,
    MENU = 2,
    GAME_OVER = 4
};

// FreeRTOS queue handles
QueueHandle_t xDirectionQueue;
QueueHandle_t xButtonQueue;
QueueHandle_t xScoreQueue;

// Game values
u8 score = 0;

int main() {
    // ------------ Initialize Devices ------------
    InitializeKeypad();

    // initialize ssd
    if (XGpio_Initialize(&SSDInst, SSD_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("GPIO initialization for SSD failed.\r\n");
        return XST_FAILURE;
    }
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
        xil_printf("GPIO initialization for buttons failed.\r\n");
        return XST_FAILURE;
    }
    XGpio_SetDataDirection(&btnInst, BTN_CHANNEL, 0x1);

    xil_printf(init_message, UP, DOWN, LEFT, RIGHT);

    // ------------ Create Queues ------------
    xDirectionQueue = xQueueCreate(DIR_QUEUE_LEN, sizeof(u8));
    xButtonQueue    = xQueueCreate(BTN_QUEUE_LEN, sizeof(u8));
    xScoreQueue     = xQueueCreate(SCORE_QUEUE_LEN, sizeof(u8));

    // ------------ Create Tasks ------------
    xTaskCreate( keypadTask                 /* The function that implements the task. */
               , "keypad task"              /* Text name for the task, provided to assist debugging only. */
               , configMINIMAL_STACK_SIZE   /* The stack allocated to the task. */
               , NULL                       /* The task parameter is not used, so set to NULL. */
               , tskIDLE_PRIORITY           /* The task runs at the idle priority. */
               , NULL
               );


    xTaskCreate( oledTask                   /* The function that implements the task. */
               , "screen task"              /* Text name for the task, provided to assist debugging only. */
               , configMINIMAL_STACK_SIZE   /* The stack allocated to the task. */
               , NULL                       /* The task parameter is not used, so set to NULL. */
               , tskIDLE_PRIORITY           /* The task runs at the idle priority. */
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
static void keypadTask(void *pvParameters) {
    (void) pvParameters;

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
            xQueueSend(xDirectionQueue, &new_key, 0);
        } else if (status == KYPD_MULTI_KEY && last_status == KYPD_SINGLE_KEY) {
            // if new key and last keys pressed overlap
            if ((keystate & last_keystate) != 0) {
                // new keys take precedent
                KYPD_getKeyPressed(&KYPDInst, (keystate & ~last_keystate), &new_key);
                xQueueSend(xDirectionQueue, &new_key, 0);
            }
        }

        last_status = status;
        last_keystate = keystate;

        vTaskDelay(xDelay);
   }
}

// Actual displaying task
static void oledTask(void *pvParameters) {
    (void) pvParameters;

    u8 current_button_state = 0; // local state for the menu
    u8 current_direction = NONE;
    u8 previous_score = 0;
    char temp[10];

    u8 incoming_btn;
    u8 incoming_dir;

    OLED_SetDrawMode(&oledDevice, 0); // draw mode == set mode
    OLED_SetCharUpdate(&oledDevice, 0); // automatic updating off

    snake_block *head = start_game();
    snake_block *consumable = create_consumable();
    
    while(1) {
        // Check for pushbutton press
        if (xQueueReceive(xButtonQueue, &incoming_btn, 0) == pdPASS) {
            current_button_state = incoming_btn;
        }

        if (xQueueReceive(xDirectionQueue, &incoming_dir, 0) == pdPASS) {
            // ensure reversal is impossible by ignoring opposites
            if ((incoming_dir == UP && current_direction != DOWN) 
            || (incoming_dir == DOWN && current_direction != UP)
            || (incoming_dir == LEFT && current_direction != RIGHT)
            || (incoming_dir == RIGHT && current_direction != LEFT)) {
                current_direction = incoming_dir;
            }
        }
        if (current_button_state == PLAY) {
            OLED_ClearBuffer(&oledDevice);

            // draw the game elements
            draw_snake(head);
            draw_snake(consumable);
            
            // update the screen
            OLED_Update(&oledDevice);
            
            // update game logic
            int is_alive = update_game(&head, &consumable, current_direction);
            
            // broadcast new score
            if (score != previous_score) {
                xQueueSend(xScoreQueue, &score, 0);
                previous_score = score;
            }

            if (is_alive == 0) {
                current_button_state = GAME_OVER; // force game over
            }
        } else if (current_button_state == MENU) {
            OLED_ClearBuffer(&oledDevice);

            // show score on the OLED
            OLED_SetCursor(&oledDevice, 0, 0);
            sprintf(temp, "Score: %d", score);
            OLED_PutString(&oledDevice, temp);

            // show time on the OLED
            OLED_SetCursor(&oledDevice, 0, 2);
            u32 ticks = xTaskGetTickCount();
            ticks = ticks / 100;
            sprintf(temp, "Time: %u", ticks);
            OLED_PutString(&oledDevice, temp);

            OLED_Update(&oledDevice);
        } else if (current_button_state == GAME_OVER) {
            game_over(&head, &consumable);
            current_direction = NONE;
            current_button_state = PLAY;
        }
        vTaskDelay(pdMS_TO_TICKS(FRAME_DELAY_MS));
    }
}

// Shows menu options
static void buttonTask(void *pvParameters) {
    (void) pvParameters;

    const TickType_t xDelay = pdMS_TO_TICKS(100); // debounce delay
    u8 buttonVal = 0;
    u8 lastButtonVal = 0;

    while(1) {
        buttonVal = XGpio_DiscreteRead(&btnInst, BTN_CHANNEL);
        
        // only send to queue if the button state actually changed
        if (buttonVal != lastButtonVal) {
            xQueueSend(xButtonQueue, &buttonVal, 0);
            lastButtonVal = buttonVal;
        }

        vTaskDelay(xDelay);
    }
}

// Displays points on the SSD
static void ssdTask(void *pvParameters) {
    (void) pvParameters;

    const TickType_t xDelay = pdMS_TO_TICKS(12);
    const u8 left_side = 0x0;
    const u8 right_side = 0x1; 

    u8 local_score = 0;
    u32 display_val = 0;

    while(1) {
        xQueueReceive(xScoreQueue, &local_score, 0);

        // display the ones digit
        display_val = SSD_decode(local_score % 10, right_side);
        XGpio_DiscreteWrite(&SSDInst, SSD_CHANNEL, display_val);
        vTaskDelay(xDelay);


        // display the tens digit
        display_val = SSD_decode((local_score / 10) % 10, left_side);
        XGpio_DiscreteWrite(&SSDInst, SSD_CHANNEL, display_val);
        vTaskDelay(xDelay);
    }
}

static u32 SSD_decode(u8 num, u8 cathode) {
    u32 result;
    
    // num == the value to display
    switch(num) { // Handles the coding of the 7-seg display
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
    if (cathode == 0) {
        return result;
    } else {
        return result | 0b10000000;
    }
}

static snake_block *start_game(void) {
    score = 0;
    snake_block *head = malloc(sizeof(snake_block));
    head->next = NULL;
    head->x = (rand() % NUM_X_CELLS) * SNAKE_BLOCK_SIZE;
    head->y = (rand() % NUM_Y_CELLS) * SNAKE_BLOCK_SIZE;

    return head;
}

static void free_snake_block(snake_block *block) {
    if (block == NULL) {
        return;
    }
    free_snake_block(block->next); // travel to the very end first
    free(block); // free the block on the way back out
}

static void draw_snake(snake_block *block) {
    // traverse the linked list
    while (block != NULL) {
        draw_block(block->x, block->y);
        block = block->next; 
    }
}

// TODO: maybe check the snake to avoid overlap
static snake_block *create_consumable(void) {
    int x_pos = (rand() % NUM_X_CELLS) * SNAKE_BLOCK_SIZE;
    int y_pos = (rand() % NUM_Y_CELLS) * SNAKE_BLOCK_SIZE;
    
    // create the consumable as a snake block
    snake_block *consumable = malloc(sizeof(snake_block));
    consumable->next = NULL;
    consumable->x = x_pos;
    consumable->y = y_pos;

    return consumable;
}

static int update_game(snake_block **head, snake_block **consumable, u8 current_direction) {
    
    // check for collision with head & consumable
    if (((*head)->x == (*consumable)->x) && ((*head)->y == (*consumable)->y)) {
        consume_point(*head, *consumable, current_direction);
        *head = *consumable; // consumable becomes new head

        *consumable = create_consumable(); // create new consumable
    } else {
        // move the snake
        move_snake(*head, current_direction);
    }

    // check for out of bounds
    // also handles underflows
    if (((*head)->x > OLED_WIDTH) || ((*head)->y > OLED_LENGTH)) {
        return 0; // snake died -> force game over
    }
    
    snake_block *tail_block = (*head)->next;
    while (tail_block != NULL) {
        // check for overlap between tail and head
        if (((*head)->x == tail_block->x) && ((*head)->y == tail_block->y)) {
            return 0;
        }
        tail_block = tail_block->next; // move down the chain
    }

    return 1; // snake is still alive
}

static inline void move_block(snake_block *block, u8 current_direction) {
    switch (current_direction) {
        case UP:    block->y -= SNAKE_BLOCK_SIZE; break;
        case DOWN:  block->y += SNAKE_BLOCK_SIZE; break;
        case LEFT:  block->x -= SNAKE_BLOCK_SIZE; break;
        case RIGHT: block->x += SNAKE_BLOCK_SIZE; break;
    }
}

static void move_snake(snake_block *block, u8 current_direction) {
    if (block == NULL) return;

    // save the head's old position before moving it
    int prev_x = block->x;
    int prev_y = block->y;

    // move the head
    move_block(block, current_direction);
    
    // ripple the old positions down the body
    snake_block *curr = block->next;
    while (curr != NULL) {
        int temp_x = curr->x;
        int temp_y = curr->y;
        
        curr->x = prev_x;
        curr->y = prev_y;
        
        prev_x = temp_x;
        prev_y = temp_y;
        
        curr = curr->next;
    }
}

static inline void consume_point(snake_block *head, snake_block *consumable, u8 current_direction) {
    ++score;
    consumable->next = head;

    consumable->x = head->x;
    consumable->y = head->y;
    move_block(consumable, current_direction);
}

static inline void draw_block(int x, int y) {
    int rect_start  = x;
    int rect_end    = x + SNAKE_BLOCK_SIZE;

    int rect_top    = y; 
    int rect_bottom = y - SNAKE_BLOCK_SIZE;

    OLED_MoveTo(&oledDevice, rect_start, rect_top);
    OLED_RectangleTo(&oledDevice, rect_end, rect_bottom);
}

static void game_over(snake_block **head, snake_block **consumable) {
    char temp[10];
    OLED_ClearBuffer(&oledDevice);

    // show "Game Over!" on the OLED
    OLED_SetCursor(&oledDevice, 0, 0);
    OLED_PutString(&oledDevice, "Game Over!");
    // show score on the OLED
    OLED_SetCursor(&oledDevice, 0, 2);
    sprintf(temp, "Score: %d", score);
    OLED_PutString(&oledDevice, temp);
    OLED_Update(&oledDevice);

    // let it fester
    vTaskDelay(pdMS_TO_TICKS(GAME_OVER_TIME_MS));

    // reset score
    score = 0;

    // free snake to avoid memory leaks
    free_snake_block(*head);
    free_snake_block(*consumable);

    // restart the pointers
    *head = start_game();
    *consumable = create_consumable();
}
