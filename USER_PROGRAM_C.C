/***
 * Title: Merged Swipe Detection and Control System for BS84D20CA
 * Description: Combines IR swipe detection (38kHz) with touch keys, LED animations, 7-segment display, and power control. Gesture priority highest.
 * Date: August 16, 2025
 ***/
#include "BS84D20CA.h"
#include "USER_PROGRAM_C.INC"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\Timer.h"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\Interrupt.h"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\LED_Driver.h"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\System_Clock.h"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\Uart.h"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\IR_Emitter.h"

// Constants (merged, with v1.4 updates)
// Key Masks (from v1.4)
#define KEY_MASK_K1 (1 << (20 - 17))
#define KEY_MASK_K2 (1 << (10 - 9))
#define KEY_MASK_K3 (1 << (19 - 17))
#define KEY_MASK_K4 (1 << (1 - 1))
#define KEY_MASK_K5 (1 << (2 - 1))
#define KEY_MASK_K6 (1 << (3 - 1))

// Assuming 25ms app_tick from main loop comment (1000ms / 25ms = 40)
//#define TICKS_PER_SECOND 40 
#define HSTATUS_COUNTDOWN_SECONDS (10 * 60) // 10 minutes = 600 seconds

#define BOOST_COUNTDOWN_SECONDS (3 * 60) // 3 minutes = 180 seconds

#define FAN_OFF_COUNTDOWN_SECONDS (1 * 60) // 1 minute = 60 seconds

#define LEVEL_DISPLAY_TICKS (3 * TICKS_PER_SECOND)  // 3 seconds
#define SPEED_DISPLAY_TICKS (5 * TICKS_PER_SECOND)  // 5 seconds

// ADDED: Globals for HStatus 10-minute countdown
volatile unsigned long h_countdown_ticks_remaining = 0;
volatile unsigned char h_countdown_tick_counter = 0;

volatile unsigned long boost_countdown_ticks_remaining = 0;
volatile unsigned char boost_countdown_tick_counter = 0;


unsigned char lstatus=0;
unsigned char istatus=0;
unsigned char dstatus=0;
unsigned char bstatus=0;
unsigned char hstatus=0;

// ADDED FOR 3-SECOND LONG PRESS ON K1
#define HSTATUS_LONG_PRESS_TICKS 60 // (120 ticks * 25ms tick = 3000ms = 3s)
static unsigned int hstatus_key_counter = 0;

// Globals (merged)
unsigned char displayBuffer[16] = {0};
volatile unsigned char key_flags[6] = {0};
unsigned char brightness_level = 1;  // Default high
unsigned char level = 0;
unsigned char boost = 0;
unsigned char previous_level = 0;
unsigned char display_timer = 0;
unsigned char show_level = 1;
const unsigned int speeds[] = {0, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400};
const LedCoord exceptionLeds[60] = { {3, 14}, {3, 10}, {3, 1}, {3, 7}, {3, 13}, {3, 12}, {3, 11}, {1, 14}, {1, 10}, {1, 1}, {1, 7}, {1, 13}, {1, 12}, {1, 11}, 
									 {2, 14}, {2, 10}, {2, 1}, {2, 7}, {2, 13}, {2, 12}, {2, 11}, {0, 14}, {0, 10}, {0, 1}, {0, 7}, {0, 13}, {0, 12}, {0, 11}, 
									 {5, 11}, {3, 8}, {1, 9}, {2, 3},{5, 14}, {4, 14}, {4, 6}, {1, 6}, {2, 6}, {3, 6}, {0, 6}, {4, 7}, {5, 7}, {4, 1}, {5, 1}, 
									 {4, 10}, {5, 10}, {3, 5}, {1, 5}, {2, 5}, {4, 5}, {0, 5}, {5, 5}, {0, 2}, {3, 2}, {2, 2}, {1, 2}, {4, 2}, {5, 13}};
									 

const LedCoord heater_exception_leds[8] = { 
    {3, 9}, {1, 9}, {2, 9}, {0, 9}, {4, 9}, {5, 9}, {5, 6}, {5, 2} 
};
								
unsigned char error_code = 0; // 0 = No Error, 1-10 corresponds to E0-E9

// Forward Declarations
void configure_io_pins(void);
void process_gesture_logic(void);
void process_touch_keys(void);
void process_k1_sequence(void);
void process_k1_shutdown_sequence(void);
void display_countdown(unsigned int seconds);  // New function for countdown display

// System Initialization (from v1.4)
void configure_io_pins(void) {
    SCLK_PORT_CTRL = 0; DIN_PORT_CTRL = 0;
    
    _pcc0=0;
    
    _pcc3 = 1; _pcpu3 = 0;  // PC3 input with pull-up
    _pcc2 = 1; _pcpu2 = 0;  // PC2 input with pull-up
    
    _pcs0 = 0b00000001;
    
    _pdc6 = 0;
    
    _pds1 = 0b10000000; // Sets PD7 as TX and PD6 as RX/TX
}

// Power Sequences (exact from v1.4, with modifications for gesture initial level)
void process_k1_sequence(void) {
    unsigned char i;
    if (k1_state != K1_STATE_INSTANT_ON && k1_state != K1_STATE_SEQUENTIAL_ON) return;
    
    const unsigned char instant_led_count = sizeof(instantOnLeds) / sizeof(LedCoord);
    const unsigned char sequential_led_count = sizeof(sequentialOnLeds) / sizeof(LedCoord);
    const unsigned char exception_count = sizeof(exceptionLeds) / sizeof(LedCoord);
    switch (k1_state) {
    	
        case K1_STATE_INSTANT_ON:
        	brightness_level = 2;
            clearDisplay();

            for (i = 0; i < instant_led_count; i++) { displayBuffer[instantOnLeds[i].grid] |= (1 << instantOnLeds[i].seg); }
            k1_sequential_index = 0;
            k1_delay_counter = K1_SEQUENTIAL_DELAY;
            k1_state = K1_STATE_SEQUENTIAL_ON;
            updateDisplay();
            break;
            
        case K1_STATE_SEQUENTIAL_ON:
            if (--k1_delay_counter == 0) {
                if (k1_sequential_index < sequential_led_count) {
                    displayBuffer[sequentialOnLeds[k1_sequential_index].grid] |= (1 << sequentialOnLeds[k1_sequential_index].seg);
                    k1_sequential_index++;
                    k1_delay_counter = K1_SEQUENTIAL_DELAY;
                } else {
                    for (i = 0; i < 16; i++) { displayBuffer[i] = 0xFF; }
                    for (i = 0; i < exception_count; i++) { turn_off_led(exceptionLeds[i].seg, exceptionLeds[i].grid); }
                    k1_state = K1_STATE_FULLY_ON;
                    if (is_gesture_poweron) {
                        level = 1;
                        is_gesture_poweron = 0;
                    } else {
                        level = 0;
                    }
                    boost = 0;
                    show_level = 1;
                    display_timer = 0;
                }
                updateDisplay();
            }
            break;
    }
}
void process_k1_shutdown_sequence(void) {
    unsigned char i;
    if (k1_state != K1_STATE_SEQUENTIAL_OFF) return;
    if (--k1_delay_counter == 0) {
        k1_delay_counter = K1_SEQUENTIAL_DELAY;
        turn_off_led(sequentialOnLeds[k1_sequential_index].seg, sequentialOnLeds[k1_sequential_index].grid);
        if (k1_sequential_index == 0) {
            const unsigned char instant_led_count = sizeof(instantOnLeds) / sizeof(LedCoord);
            for (i = 0; i < instant_led_count; i++) {
                turn_off_led(instantOnLeds[i].seg, instantOnLeds[i].grid);
            }
            k1_state = K1_STATE_OFF;
            clearDisplay();
            updateDisplay();
            brightness_level = 1;
            display_standby_pattern();
            
            off_sequence_state = 0;  // Reset off sequence on shutdown
            
            hstatus = 0;
            h_countdown_ticks_remaining = 0;
            if (switchstatus & 0x08) { // Only clear bit if it was set
                 switchstatus &= 0xF7;
            }
            turn_off_led(3, 8);
            
            // Also clear boost state and timer on shutdown
            boost = 0;
            bstatus = 0;
            boost_countdown_ticks_remaining = 0;
            if (switchstatus & 0x04) {
                 switchstatus &= 0xFB;
            }
            
        } else {
            k1_sequential_index--;
        }
        updateDisplay();
    }
}

// Add this entire new function
/*void handle_critical_error(unsigned int errorCode) {
    unsigned char detected_error_code = 0;

    // Parse the errorCode according to the bitmask
    if (errorCode & 1)      detected_error_code = 2; // E1
    else if (errorCode & 2) detected_error_code = 3; // E2
    else if (errorCode & 4) detected_error_code = 4; // E3
    else if (errorCode & 8) detected_error_code = 5; // E4
    else if (errorCode & 16) detected_error_code = 6; // E5
    else if (errorCode & 32) detected_error_code = 7; // E6
    else if (errorCode & 64) detected_error_code = 8; // E7
    else if (errorCode & 128) detected_error_code = 9; // E8
    // This can be extended for all 16 bits if needed

    // If a valid error was found, lock the system
    if (detected_error_code > 0) {
        error_code = detected_error_code;

        // --- CRITICAL ERROR LOCKDOWN ---
        // The program will be trapped in this loop until the power is cycled.
        while(1) {
            display_seven_seg(); // This will now exclusively show the error
            updateDisplay();
            GCC_CLRWDT(); // Reset the watchdog timer to prevent a system reset
        }
    }
}*/

// New: Display countdown on 4-digit display (e.g., "0100" for 100 seconds)
/*void display_countdown(unsigned int seconds) {
    clear_custom_number_area();
    unsigned char i;
    unsigned char digits[4];
    digits[0] = (seconds / 1000) % 10;  // Thousands
    digits[1] = (seconds / 100) % 10;   // Hundreds
    digits[2] = (seconds / 10) % 10;    // Tens
    digits[3] = seconds % 10;           // Ones
    for (i = 0; i < 4; i++) {
        display_digit(i, digits[i]);
    }
    updateDisplay();
}*/

// New: Display countdown on 4-digit display (e.g., "10 00" for 10 minutes)
void display_countdown(unsigned int total_seconds) {
    clear_custom_number_area();
    unsigned char i;
    unsigned char digits[4];

    // Convert total seconds to minutes and remaining seconds
    unsigned int minutes = total_seconds / 60;
    unsigned int seconds = total_seconds % 60;

    // Split into four digits for the display (MMSS)
    digits[0] = (minutes / 10) % 10;   // Minutes (Tens)
    digits[1] = minutes % 10;          // Minutes (Ones)
    digits[2] = (seconds / 10) % 10;   // Seconds (Tens)
    digits[3] = seconds % 10;          // Seconds (Ones)

    for (i = 0; i < 4; i++) {
        display_digit(i, digits[i]);
    }
    
    // Optionally, if your display driver supports it,
    // you might want to light up a colon or dot between digits 1 and 2
    // e.g., turn_on_led(COLON_SEGMENT, COLON_GRID);

    updateDisplay();
}

void process_gesture_logic(void) {
    // Clear flags before process
    unsigned char left_detected = (IR_RECEIVER_1 && !IR_RECEIVER_2);
    unsigned char right_detected = (IR_RECEIVER_2 && !IR_RECEIVER_1);

    // Idle state condition
    if (gesture_state == GESTURE_IDLE) {
        if (left_detected && gesture_cooldown_counter == 0) {
            gesture_state = GESTURE_ARMED_LEFT_START;
            gesture_timeout_counter = 0;
        } else if (right_detected && gesture_cooldown_counter == 0) {
            gesture_state = GESTURE_ARMED_RIGHT_START;
            gesture_timeout_counter = 0;
        }
    }

    // Detect left to right swipe: left_detected first then right_detected
    if (gesture_state == GESTURE_ARMED_LEFT_START) {
        if (right_detected) {
            // Confirm full swipe from left to right
            gesture_state = GESTURE_COMPLETED_LEFT_TO_RIGHT;
            gesture_cooldown_counter = GESTURE_COOLDOWN;

            // Implement swipe actions
            perform_right_to_left_action();
            
            if (boost) { // Check if boost is active
                level = 0; // Set level to 0
                
                // Also deactivate boost mode and stop its timer
                boost = 0;
                bstatus = 0;
                boost_countdown_ticks_remaining = 0;
                if (switchstatus & 0x04) {
                     switchstatus &= 0xFB;
                }

                // Make sure the display updates to "0"
                show_level = 1;
                display_timer = 0;
            }

            gesture_state = GESTURE_IDLE;
        } else {
            if (++gesture_timeout_counter > GESTURE_TIMEOUT) {
                // Timeout, reset
                gesture_state = GESTURE_IDLE;
            }
        }
    }

    // Detect right to left swipe: right_detected first then left_detected
    if (gesture_state == GESTURE_ARMED_RIGHT_START) {
        if (left_detected) {
            // Confirm full swipe from right to left
            gesture_state = GESTURE_COMPLETED_RIGHT_TO_LEFT;
            gesture_cooldown_counter = GESTURE_COOLDOWN;

            // Implement swipe actions
            
            
            perform_left_to_right_action();

            gesture_state = GESTURE_IDLE;
        } else {
            if (++gesture_timeout_counter > GESTURE_TIMEOUT) {
                // Timeout, reset
                gesture_state = GESTURE_IDLE;
            }
        }
    }
}

void process_touch_keys(void) {
    if(SCAN_CYCLEF){
        GET_KEY_BITMAP();
        // Swapped: Original K1 action now on K6 (power on/off sequence)
        if((DATA_BUF[0] & KEY_MASK_K6) && !key_flags[5]){
            key_flags[5] = 1;
            // level=0; // <-- This is now conditional
            toggle_led(5, 4); // This LED will be handled by the blink logic if timer starts

            if (switchstatus & 0x01)
            {
                switchstatus &= 0xFE;
            }
            else
            {
                switchstatus |= 0x01;
            }

            // --- NEW POWER OFF LOGIC ---
            if (k1_state == K1_STATE_OFF) { 
                // --- Powering ON ---
                k1_state = K1_STATE_INSTANT_ON;
                off_sequence_state = 0; // Ensure timer state is reset
                countdown_ticks_remaining = 0;
            
            } else if (k1_state == K1_STATE_FULLY_ON) {
                // --- Powering OFF ---
                
                if (level != 0 && off_sequence_state == 0) {

                    
                    off_sequence_state = 1; // Start timer state
                    countdown_ticks_remaining = (unsigned long)FAN_OFF_COUNTDOWN_SECONDS * TICKS_PER_SECOND;
                    countdown_tick_counter = 0; // Reset per-second counter
                    
                    // Note: We DO NOT set k1_state to OFF. The timer in the main loop will do this.

                } else {
                    // Level is ALREADY 0 (or timer is running): Shut down immediately
                    off_sequence_state = 0; // Ensure timer is off
                    countdown_ticks_remaining = 0;
                    level=0;
                    
                    k1_state = K1_STATE_SEQUENTIAL_OFF;
                    k1_sequential_index = (sizeof(sequentialOnLeds) / sizeof(LedCoord)) - 1;
                    k1_delay_counter = K1_SEQUENTIAL_DELAY;
                }
            }
            // --- END NEW LOGIC ---

        } else if(!(DATA_BUF[0] & KEY_MASK_K6)){ key_flags[5]=0; }
        // Swapped: Original K2 action now on K5 (change brightness)
        if((DATA_BUF[0] & KEY_MASK_K5) && !key_flags[4] /*&& k1_state == K1_STATE_FULLY_ON*/){
            key_flags[4]=1;

            if (switchstatus & 0x02)
            {
                switchstatus &= 0xFD;
                lstatus=0;                
            }
            else
            {
                switchstatus |= 0x02;
                lstatus=1;
            }


            /*brightness_level = (brightness_level + 1) % 3;
            switch (brightness_level) {
                case 0: sendCommand(CMD_DISPLAY_ON_HIGH); break;
                case 1: sendCommand(CMD_DISPLAY_ON_HIGH); break;
                case 2: sendCommand(CMD_DISPLAY_ON_HIGH); break;
            }*/
        } else if(!(DATA_BUF[0] & KEY_MASK_K5)){ key_flags[4]=0; }
        // Swapped: Original K3 action now on K4 (increment level)
        if((DATA_BUF[0] & KEY_MASK_K4) && !key_flags[3] && k1_state == K1_STATE_FULLY_ON && hstatus!=1){
            key_flags[3]=1;
            
            if (off_sequence_state == 1) {
                off_sequence_state = 0;
                countdown_ticks_remaining = 0;
                clear_custom_number_area(); // Clear the timer from the 7-seg display
                turn_off_led(5, 4);         // Ensure timer LED is off
            }
            
            istatus=1; 
            
            turn_off_led(5,2);
            
            
            if (level < 9) { // Increment only if not at max
                level++;
            }
            show_level = 1;
            display_timer = 0;

        } else if(!(DATA_BUF[0] & KEY_MASK_K4)){ key_flags[3]=0; }
        // Swapped: Original K4 action now on K3 (decrement level)
        if((DATA_BUF[2] & KEY_MASK_K3) && !key_flags[2] && k1_state == K1_STATE_FULLY_ON && hstatus!=1){
            key_flags[2]=1;
            
            if (off_sequence_state == 1) {
                off_sequence_state = 0;
                countdown_ticks_remaining = 0;
                clear_custom_number_area(); // Clear the timer from the 7-seg display
                turn_off_led(5, 4);         // Ensure timer LED is off
            }
            
            dstatus=1;
            
            turn_off_led(5,6);
            

            if (level > 0) { // Decrement only if not at min
                level--;
            }
            show_level = 1;
            display_timer = 0;


        } else if(!(DATA_BUF[2] & KEY_MASK_K3)){ key_flags[2]=0; }
		// Swapped: Original K5 action now on K2 (boost toggle)
		if((DATA_BUF[1] & KEY_MASK_K2) && !key_flags[1] && k1_state == K1_STATE_FULLY_ON && hstatus!=1){
		    key_flags[1]=1;
		    
		    if (off_sequence_state == 1) {
                off_sequence_state = 0;
                countdown_ticks_remaining = 0;
                clear_custom_number_area(); // Clear the timer from the 7-seg display
                turn_off_led(5, 4);         // Ensure timer LED is off
            }
		
		    if (switchstatus & 0x4)
		    {
		        switchstatus &= 0xFB;
		        bstatus=0;
		    }
		    else
		    {
		        switchstatus |= 0x04;
		        bstatus=1;
		    }
		
		    boost = !boost;
		    if (boost) {
		        previous_level = level;
		        level = 10;
		        
		        // --- ADD THIS ---
		        // Start 30-minute secret timer
		        boost_countdown_ticks_remaining = (unsigned long)BOOST_COUNTDOWN_SECONDS * TICKS_PER_SECOND;
		        boost_countdown_tick_counter = 0;
		        // --- END ADD ---
		
		    } else {
		        level = previous_level;
		
		        // --- ADD THIS ---
		        // Stop timer
		        boost_countdown_ticks_remaining = 0;
		        // --- END ADD ---
		    }
		
		    if (level == 0) show_level = 1;
		    else {
		        show_level = 1;
		        display_timer = 0;
		    }
		    
		} else if(!(DATA_BUF[1] & KEY_MASK_K2)){ key_flags[1]=0; }
        // Swapped: Original K6 action now on K1 (toggle specific LED)
        // --- MODIFIED FOR 3-SECOND LONG PRESS (NO SHORT PRESS) ---

        // Check if key is PHYSICALLY pressed
        if(DATA_BUF[2] & KEY_MASK_K1)
        {
            // Key is HELD.
            // Check if system state is valid AND the action hasn't already run
            if (k1_state == K1_STATE_FULLY_ON && level==0 && !key_flags[0])
            {
                // System state is valid. Start/continue counting.
                if (hstatus_key_counter < HSTATUS_LONG_PRESS_TICKS)
                {
                    hstatus_key_counter++;
                }

                // Check if we *just* hit the 3-second mark
                if (hstatus_key_counter == HSTATUS_LONG_PRESS_TICKS)
                {
                    // --- 3-SECOND LONG-PRESS ACTION ---
                    if (switchstatus & 0x08)
                    {
                        switchstatus &= 0xF7;
                        hstatus=0;
                        turn_off_led(3, 8);

                        // Stop and clear the hstatus countdown
                        h_countdown_ticks_remaining = 0;
                        if (off_sequence_state != 1) { // Only clear display if other timer isn't active
                            clear_custom_number_area();
                            updateDisplay();
                        }
                    }
                    else
                    {
                        switchstatus |= 0x08;
                        hstatus=1;
                        turn_on_led(3, 8);
                        
                        // Start the 10-minute countdown
                        h_countdown_ticks_remaining = (unsigned long)HSTATUS_COUNTDOWN_SECONDS * TICKS_PER_SECOND;
                        h_countdown_tick_counter = 0; // Reset per-second counter
                    }
                    // --- END OF ACTION ---
                    
                    key_flags[0] = 1; // Mark action as done (using your existing flag)
                }
            }
            // If key is held but system state is invalid (e.g., level != 0), reset the counter
            else if (k1_state != K1_STATE_FULLY_ON || level!=0)
            {
                hstatus_key_counter = 0; // Reset counter
            }
        } 
        else
        {
            // Key is PHYSICALLY released.
            // Reset all flags and counters.
            hstatus_key_counter = 0;
            key_flags[0] = 0; // Reset the "action done" flag
        }
        // --- END OF LONG PRESS MODIFICATION ---
    }
}

// Main Initialization (exact from v1.4)
void USER_PROGRAM_C_INITIAL(void) {
    configure_system_clock();
    configure_io_pins();
    init_PTM_for_tick();
    initialize_gesture();
    init_uart();
    display_standby_pattern();
    init_STM();
    //init_CTM();
    
    // Touch Key Configuration
    _tkm0c0 = (0b1001 << 4) | (0b000 << 3) | 1;
    _tkm0c1 = (0b00 << 6) | (0b00 << 2);
    _tkm1c0 = 0b00000111; _tkm1c1 = 0b00000000;
    _tkm2c0 = 0b00000010; _tkm2c1 = 0b00000000;
    _tkm3c0 = 0b00001100; _tkm3c1 = 0b00000000;
    _wdtc = 0b01010111;
    
	//_ctmae = 1;
    _stmae = 1;
    _ptmae = 1;
    //_mfe = 1;
    _emi = 1;
    
}

// Main Loop (exact from v1.4, with gesture priority via ISR and countdown handling)
void USER_PROGRAM_C(void) {
    if (app_tick_flag) {
        app_tick_flag = 0;
        
        if (gesture_cooldown_counter > 0) gesture_cooldown_counter--;
        
        process_gesture_logic();
        process_touch_keys();
        process_k1_sequence();
        process_k1_shutdown_sequence();
        
        // Handle countdown for off sequence (every 25ms tick)
        if (off_sequence_state == 1 && countdown_ticks_remaining > 0 && hstatus != 1) {
            countdown_ticks_remaining--;
            countdown_tick_counter++;
            
            if (countdown_tick_counter < (TICKS_PER_SECOND / 2)) { // First 20 ticks (0.5s)
                 turn_on_led(5, 4);
            } else { // Last 20 ticks (0.5s)
                 turn_off_led(5, 4);
            }
            
            
            if (countdown_tick_counter >= TICKS_PER_SECOND) {  // Every second
                countdown_tick_counter = 0;
                unsigned int remaining_seconds = (countdown_ticks_remaining + (TICKS_PER_SECOND - 1)) / TICKS_PER_SECOND;  // Round up for display
                display_countdown(remaining_seconds);
            }
            if (countdown_ticks_remaining == 0) {
                off_sequence_state = 0;
                k1_state = K1_STATE_SEQUENTIAL_OFF;
                k1_sequential_index = (sizeof(sequentialOnLeds) / sizeof(LedCoord)) - 1;
                k1_delay_counter = K1_SEQUENTIAL_DELAY;
                clear_custom_number_area();  // Clear display after countdown
                level=0;
                updateDisplay();
            }
        }
        
        if (hstatus == 1 && h_countdown_ticks_remaining > 0 && off_sequence_state != 1) {
            h_countdown_ticks_remaining--;
            h_countdown_tick_counter++;
            
            if (h_countdown_tick_counter < (TICKS_PER_SECOND / 2)) { // First 20 ticks (0.5s)
                 turn_on_led(3, 8);
            } else { // Last 20 ticks (0.5s)
                 turn_off_led(3, 8);
            }
            
            if (h_countdown_tick_counter >= TICKS_PER_SECOND) {  // Every second
                h_countdown_tick_counter = 0;
                unsigned int remaining_seconds = (h_countdown_ticks_remaining + (TICKS_PER_SECOND - 1)) / TICKS_PER_SECOND;  // Round up for display
                display_countdown(remaining_seconds);
            }
            if (h_countdown_ticks_remaining == 0) {
                // Timer finished, turn off hstatus
                hstatus = 0;
                switchstatus &= 0xF7; // Clear the switch status bit
                turn_off_led(3, 8);    // Toggle the LED back off
                
                clear_custom_number_area();  // Clear display after countdown
                updateDisplay();
            }
        }
        
        if (boost_countdown_ticks_remaining > 0) {
		    boost_countdown_ticks_remaining--;
		    
		    boost_countdown_tick_counter++;
            if (boost_countdown_tick_counter >= TICKS_PER_SECOND) {
                boost_countdown_tick_counter = 0; // Reset for next second
            }
		    
		    if (boost_countdown_ticks_remaining == 0) {
		        // Timer finished, drop from Boost (level 10) to level 9
		        if (level == 10 && boost == 1) { // Check if we are still in boost mode
		            level = 9;
		            boost = 0;
		            bstatus = 0;
		            switchstatus &= 0xFB; // Clear the switch status bit
		            
		            show_level = 1; // Show the new level "9"
		            display_timer = 0;
		        }
		    }
		}
		
		if (k1_state == K1_STATE_FULLY_ON) {

            if (hstatus == 1) {
                // Heater ON: Force all 8 exception LEDs OFF.
                unsigned char i;
                const unsigned char heater_exception_count = sizeof(heater_exception_leds) / sizeof(LedCoord);
                for (i = 0; i < heater_exception_count; i++) {
                    turn_off_led(heater_exception_leds[i].seg, heater_exception_leds[i].grid);
                }
            } 
            else {
                // Heater OFF: Restore default ON state.
                
                // 1. Restore the 5 LEDs that have no other logic
                turn_on_led(3, 9);
                turn_on_led(2, 9);
                turn_on_led(0, 9);
                turn_on_led(4, 9);
                turn_on_led(5, 9);

                // 2. Restore LEDs (5,2) and (5,6) ONLY if they are not
                //    in their "blink-off" cycle (istatus/dstatus == 1).

                    turn_on_led(5, 2);


                    turn_on_led(5, 6);


				if (level == 10) { // This means boost timer is running
                    // Timer is running, so blink.
                    if (boost_countdown_tick_counter < (TICKS_PER_SECOND / 2)) { // First half second
                        turn_on_led(1, 9);
                    } else { // Second half second
                        turn_off_led(1, 9);
                    }
                } else {
                    turn_off_led(1, 9);
                }
            }
        }

        if (k1_state == K1_STATE_FULLY_ON && level != 0 && off_sequence_state != 1 && hstatus != 1) {
            
            if (show_level == 1) { 
                // State 1: Showing Level for 3s
                if (++display_timer >= LEVEL_DISPLAY_TICKS) {
                    show_level = 0; // Switch to Speed state
                    display_timer = 0; // Reset timer
                }
            } else if (show_level == 0) { 
                // State 0: Showing Speed for 5s
                if (++display_timer >= SPEED_DISPLAY_TICKS) {
                    show_level = 2; // Switch to permanent Level state
                }
            }
            // If show_level == 2, do nothing, just stay in permanent Level state.
        }
        
        if (k1_state == K1_STATE_FULLY_ON && off_sequence_state != 1 && hstatus != 1) {  // Skip level/speed display during EITHER countdown
            display_seven_seg();
        }
        
        
        updateDisplay();
        
        if(lstatus==1)
	        {
				turn_on_led(2, 3);
	        }
		else
			{
				turn_off_led(2,3);
			}
			

    }
    

    	
	      uart_t_byte();
	      UartRxDataProcess();
      
      
    if (heartbeat_flag) { 
        heartbeat_flag = 0; 
        updateDisplay();
        
	     	
    }
    
    GCC_CLRWDT();
    
}
void USER_PROGRAM_C_HALT_PREPARE(void) {}
void USER_PROGRAM_C_HALT_WAKEUP(void) {}
void USER_PROGRAM_C_RETURN_MAIN(void) {}
