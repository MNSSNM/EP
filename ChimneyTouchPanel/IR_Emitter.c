#include "IR_Emitter.h"
#include "BS84D20CA.h"
#include "Interrupt.h"
#include "IR_Emitter.h"
#include "Led_Driver.h"
#include "Uart.h"

// Pin Configuration
#define IR_EMITTER_PORT     _pc0
#define IR_EMITTER_CTRL     _pcc0

extern unsigned char lstatus;

// No specific functions in the original code beyond defines and ISR usage.
// Add initialization if needed.
void init_IR_Emitter(void) {
    IR_EMITTER_CTRL = 0;  // Set as output
    IR_EMITTER_PORT = 0;  // Initial state
}

// New globals for gesture changes and countdown
 unsigned char off_sequence_state ;  // 0: normal, 1: in countdown, 2: at l0
 unsigned int countdown_ticks_remaining ;  // Countdown tick counter
 unsigned char is_gesture_poweron ;
 volatile unsigned int countdown_tick_counter ;  // For per-second updates

// <<< START OF CHANGES >>>
// Definitions for the global gesture state variables
volatile unsigned char gesture_state = GESTURE_IDLE;
volatile unsigned int gesture_timeout_counter = 0;
volatile unsigned int gesture_cooldown_counter = 0;
// <<< END OF CHANGES >>>


// Gesture Initialization and Reset
void initialize_gesture(void) {
    gesture_state = GESTURE_IDLE;
    gesture_cooldown_counter = 0;
    gesture_timeout_counter = 0;
    // Clear any IR receiver states if applicable
    // Example: atomic clear IR capture registers if hardware access allowed
    // Clear pending interrupt flags again for safety
    _int0f = 0;
    _int1f = 0;
}

void reset_gesture_state(void) {
    gesture_state = GESTURE_IDLE;
    gesture_cooldown_counter = GESTURE_COOLDOWN;  // Start cooldown
    gesture_timeout_counter = 0;
}

// New: Actions for full swipes
void perform_left_to_right_action(void) {
    
    if (k1_state == K1_STATE_FULLY_ON && hstatus==0) {
    	
    	if (level == 0) {
            off_sequence_state = 0; // Reset state
            k1_state = K1_STATE_SEQUENTIAL_OFF;
            k1_sequential_index = (sizeof(sequentialOnLeds) / sizeof(LedCoord)) - 1;
            k1_delay_counter = K1_SEQUENTIAL_DELAY;

            // Also clear lstatus (light) if it was on
                 switchstatus &= 0xFD;
                 lstatus=0;
                 turn_off_led(2,3);
				 level=0;
        } 
        // If level > 0, use the multi-state timer logic.
        else {
    	
	        switch (off_sequence_state) {
	            case 0:
	                off_sequence_state = 1;
	                countdown_ticks_remaining = COUNTDOWN_TOTAL_TICKS;
	                countdown_tick_counter = 0;
	                break;
	            
	            // --- MODIFIED CASE ---
	            // This now turns off immediately instead of showing L0
	            case 1:
	                off_sequence_state = 0; // Reset state
	                k1_state = K1_STATE_SEQUENTIAL_OFF;
	                k1_sequential_index = (sizeof(sequentialOnLeds) / sizeof(LedCoord)) - 1;
	                k1_delay_counter = K1_SEQUENTIAL_DELAY;
	
	                    switchstatus &= 0xFD;
	                    lstatus=0;
	                    turn_off_led(2,3);
	                    level=0;
						
						
	                break;
	            // --- END MODIFICATION ---
	        }
        
        }
    }
}

void perform_right_to_left_action(void) {
	
	if(hstatus==0){
	
		if(k1_state == K1_STATE_OFF)
		{
			if(switchstatus != 0x02)
			{
				switchstatus |= 0x02;
				lstatus=1;
			}
		}
		
	    if (off_sequence_state == 1 || off_sequence_state == 2) {
	        // Reset countdown or L0 on right-to-left swipe
	        off_sequence_state = 0;
	        countdown_ticks_remaining = 0;
	        countdown_tick_counter = 0;
	        clear_custom_number_area();  // Clear countdown display
	        updateDisplay();
	        // Restore previous state (e.g., turn back on if was in L0)
	        if (k1_state == K1_STATE_FULLY_ON) {
	            level = previous_level ? previous_level : 1;  // Restore or set to 1
	            show_level = 1;
	            display_timer = 0;
	        } else {
	            is_gesture_poweron = 1;
	            k1_state = K1_STATE_INSTANT_ON;
	        }
	    } else if (k1_state == K1_STATE_OFF) {
	        is_gesture_poweron = 1;
	        k1_state = K1_STATE_INSTANT_ON;
	    } else if (k1_state == K1_STATE_FULLY_ON) {
	        if (!boost) {
	            level = (level == 9) ? 0 : level + 1;
	            show_level = 1;
	            display_timer = 0;
	        }
	    }
	}
}