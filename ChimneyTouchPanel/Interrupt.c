#include "Interrupt.h"
#include "BS84D20CA.h"
#include "IR_Emitter.h"
#include "Led_Driver.h"
#include "Uart.h"

// Forward declarations (assuming these are defined elsewhere)
const LedCoord sequentialOnLeds[25] = { /*{5, 14}, {4, 14}, {4, 6}, {1, 6}, {2, 6}, {3, 6}, {0, 6}, {4, 7}, {5, 7}, {4, 1}, {5, 1}, {4, 10}, {5, 10}, {3, 5}, {1, 5}, {2, 5}, {4, 5}, {0, 5}, {5, 5}, {0, 2}, {3, 2}, {2, 2}, {1, 2}, {4, 2}, {5, 13}*/ };

volatile unsigned char k1_state=K1_STATE_OFF;
unsigned char k1_sequential_index;
unsigned char k1_delay_counter;
void turn_on_led(unsigned char seg, unsigned char grid);
volatile unsigned char app_tick_flag = 0;
volatile unsigned char heartbeat_flag = 0;
static unsigned char app_tick_counter = 0;
volatile unsigned int heartbeat_counter = 0, RXTimeout;

volatile unsigned char blink_flag = 0; // Initialize the new flag
static unsigned int blink_counter = 0; // Counter for the blink timer

volatile unsigned char blink_flag1 = 0; // Initialize the new flag
static unsigned int blink_counter1 = 0; // Counter for the blink timer


// CTM ISR 
void __attribute__((interrupt(0x14))) CTM_Timer_ISR(void) {
/* 
    if (_ctmaf) {
        _ctmaf = 0;        
    }
    
    if (++blink_counter >= 2000)
    {
        blink_counter = 0;
        blink_flag = 1;
    }
    
    if (++blink_counter1 >= 2000)
    {
        blink_counter1 = 0;
        blink_flag1 = 1;
    }
*/
}



// PTM ISR 
void __attribute__((interrupt(0x1C))) PTM_Timer_ISR(void) {
    

    if (_ptmaf) _ptmaf = 0;
    
    if(RXTimeout)
     RXTimeout--;
     
    if (++app_tick_counter >= TICKS_PER_APP_TICK) 
    {
        app_tick_counter = 0;
        app_tick_flag = 1;
    }
    
    if (++heartbeat_counter >= 500) 
    {
        heartbeat_counter = 0;
        heartbeat_flag = 1;
		tready = 1;
		_pdc6 = 0;
	    _pds1 &= 0b11011111; 
    }
    
    
}


