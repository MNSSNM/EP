/***
 * Title: Merged Swipe Detection and Control System for BS84D20CA
 * Description: Combines IR swipe detection (38kHz) with touch keys, LED animations, 7-segment display, and power control. Gesture priority highest.
 * Date: August 16, 2025
 ***/
#include "BS84D20CA.h"
#include "USER_PROGRAM_C.INC"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\PeriodicTimer.h"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\UART.h"
#include "C:\Users\ifb_d\Desktop\Software Development\Embedded\ChimneyTouchPanel\ChimneyTouchPanel\Led_Driver.h"

// Pin Configuration (from verified base + v1.4)
#define IR_EMITTER_PORT     _pc0
#define IR_EMITTER_CTRL     _pcc0
#define IR_RECEIVER_1       _pc3  // Left (PC3, active high)
#define IR_RECEIVER_2       _pc2  // Right (PC2, active high)
#define SCLK_PORT           _pc6
#define SCLK_PORT_CTRL      _pcc6
#define DIN_PORT            _pc7
#define DIN_PORT_CTRL       _pcc7

// Constants (merged, with v1.4 updates)
#define STM_PERIOD          26     // For ~38kHz toggle (fSYS=8MHz)
#define TICKS_PER_APP_TICK  39     // ~20ms
#define TICKS_PER_HEARTBEAT 1000     // ~25ms
#define DISPLAY_DELAY_TICKS 100    // ~2s
#define GESTURE_TIMEOUT     15
#define GESTURE_COOLDOWN    25
#define K1_SEQUENTIAL_DELAY 1
#define CMD_DATA_AUTO_INC   0x40
#define CMD_ADDRESS_SET     0xC0
#define CMD_DISPLAY_ON_LOW  0x88
#define CMD_DISPLAY_ON_MED  0x8B
#define CMD_DISPLAY_ON_HIGH 0x8F

// Key Masks (from v1.4)
#define KEY_MASK_K1 (1 << (20 - 17))
#define KEY_MASK_K2 (1 << (10 - 9))
#define KEY_MASK_K3 (1 << (19 - 17))
#define KEY_MASK_K4 (1 << (1 - 1))
#define KEY_MASK_K5 (1 << (2 - 1))
#define KEY_MASK_K6 (1 << (3 - 1))

// Enums (from v1.4, defined early)
enum { GESTURE_IDLE, GESTURE_ARMED_R1, GESTURE_ARMED_R2 };
enum { K1_STATE_OFF, K1_STATE_INSTANT_ON, K1_STATE_SEQUENTIAL_ON, K1_STATE_FULLY_ON, K1_STATE_SEQUENTIAL_OFF };

// Globals (merged)
unsigned char displayBuffer[16] = {0};
volatile unsigned char gesture_state = GESTURE_IDLE;
unsigned char gesture_timeout_counter = 0;
unsigned char gesture_cooldown_counter = 0;
volatile unsigned char app_tick_flag = 0;
volatile unsigned char heartbeat_flag = 0;
volatile unsigned char key_flags[6] = {0};
volatile unsigned char k1_state = K1_STATE_OFF;
unsigned char k1_sequential_index = 0;
unsigned char k1_delay_counter = 0;
unsigned char brightness_level = 2;  // Default high
unsigned char level = 0;
unsigned char boost = 0;
unsigned char previous_level = 0;
unsigned char display_timer = 0;
unsigned char show_level = 1;
const unsigned int speeds[] = {0, 500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400};
// --- LED COORDINATE DEFINITIONS ---
typedef struct { unsigned char seg; unsigned char grid; } LedCoord;
const LedCoord instantOnLeds[] = { {3, 8}, {1, 8}, {4, 8}, {2, 8}, {5, 8}, {0, 8} };
const LedCoord sequentialOnLeds[] = { {5, 14}, {4, 14}, {4, 6}, {1, 6}, {2, 6}, {3, 6}, {0, 6}, {4, 7}, {5, 7}, {4, 1}, {5, 1}, {4, 10}, {5, 10}, {3, 5}, {1, 5}, {2, 5}, {4, 5}, {0, 5}, {5, 5}, {0, 2}, {3, 2}, {2, 2}, {1, 2}, {4, 2}, {5, 13} };
const LedCoord exceptionLeds[] = { {3, 14}, {3, 10}, {3, 1}, {3, 7}, {3, 13}, {3, 12}, {3, 11}, {1, 14}, {1, 10}, {1, 1}, {1, 7}, {1, 13}, {1, 12}, {1, 11}, {2, 14}, {2, 10}, {2, 1}, {2, 7}, {2, 13}, {2, 12}, {2, 11}, {0, 14}, {0, 10}, {0, 1}, {0, 7}, {0, 13}, {0, 12}, {0, 11}, {5, 11} };
// --- FINAL 7-SEGMENT MAPPING (Based on user-provided map) ---
const LedCoord d1_sA = {1, 14}; const LedCoord d1_sB = {1, 13}; const LedCoord d1_sC = {1, 1};  const LedCoord d1_sD = {1, 10}; const LedCoord d1_sE = {1, 7};  const LedCoord d1_sF = {1, 12}; const LedCoord d1_sG = {1, 11};
const LedCoord d2_sA = {2, 14}; const LedCoord d2_sB = {2, 13}; const LedCoord d2_sC = {2, 1};  const LedCoord d2_sD = {2, 10}; const LedCoord d2_sE = {2, 7};  const LedCoord d2_sF = {2, 12}; const LedCoord d2_sG = {2, 11};
const LedCoord d3_sA = {3, 14}; const LedCoord d3_sB = {3, 13}; const LedCoord d3_sC = {3, 1};  const LedCoord d3_sD = {3, 10}; const LedCoord d3_sE = {3, 7};  const LedCoord d3_sF = {3, 12}; const LedCoord d3_sG = {3, 11};
const LedCoord d4_sA = {0, 14}; const LedCoord d4_sB = {0, 13}; const LedCoord d4_sC = {0, 1};  const LedCoord d4_sD = {0, 10}; const LedCoord d4_sE = {0, 7};  const LedCoord d4_sF = {0, 12}; const LedCoord d4_sG = {0, 11};
const LedCoord* const all_seven_seg_leds[] = {
    &d1_sA, &d1_sB, &d1_sC, &d1_sD, &d1_sE, &d1_sF, &d1_sG, &d2_sA, &d2_sB, &d2_sC, &d2_sD, &d2_sE, &d2_sF, &d2_sG,
    &d3_sA, &d3_sB, &d3_sC, &d3_sD, &d3_sE, &d3_sF, &d3_sG, &d4_sA, &d4_sB, &d4_sC, &d4_sD, &d4_sE, &d4_sF, &d4_sG
};
const LedCoord* const digit_a_map[7] = { &d1_sD, &d1_sE, &d1_sF, &d1_sA, &d1_sB, &d1_sC, &d1_sG };
const LedCoord* const digit_b_map[7] = { &d2_sD, &d2_sE, &d2_sF, &d2_sA, &d2_sB, &d2_sC, &d2_sG };
const LedCoord* const digit_c_map[7] = { &d3_sD, &d3_sE, &d3_sF, &d3_sA, &d3_sB, &d3_sC, &d3_sG };
const LedCoord* const digit_d_map[7] = { &d4_sD, &d4_sE, &d4_sF, &d4_sA, &d4_sB, &d4_sC, &d4_sG };
const LedCoord* const* const digit_place_map[4] = {
    digit_d_map, // Position 0 (Thousands place)
    digit_c_map,  // Position 1 (Hundreds place)
    digit_b_map, // Position 2 (Tens place)
    digit_a_map // Position 3 (Ones place)
};
const unsigned char seven_seg_font[11] = { 0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111, 0b0111000 }; // Added index 10 for 'L' (0b0111000)

// Forward Declarations
void configure_system_clock(void);
void configure_io_pins(void);
void init_STM(void);
void init_PTM_for_tick(void);
void updateDisplay(void);
void turn_on_led(unsigned char seg, unsigned char grid);
void precise_delay_us(unsigned int us);
void process_gesture_logic(void);
void turn_off_led(unsigned char seg, unsigned char grid);
void toggle_led(unsigned char seg, unsigned char grid);
void clear_custom_number_area(void);
void display_seven_seg(void);
void display_digit(unsigned char pos, unsigned char val);
void process_touch_keys(void);
void process_k1_sequence(void);
void process_k1_shutdown_sequence(void);
void clearDisplay(void);
void start(void);
void stop(void);
void writeByte(unsigned char data);
void sendCommand(unsigned char cmd);

// Missing handlers (from v1.4)
void USER_PROGRAM_C_HALT_PREPARE(void) {}
void USER_PROGRAM_C_HALT_WAKEUP(void) {}
void USER_PROGRAM_C_RETURN_MAIN(void) {}

// STM ISR: Gesture priority highest (from verified + v1.4 swipe-to-power)
void __attribute__((interrupt(0x34))) Multi_Function_ISR(void) {
    unsigned char r1_active = IR_RECEIVER_1;
    unsigned char r2_active = IR_RECEIVER_2;
    if (_stmaf) {
        _stmaf = 0;
        IR_EMITTER_PORT = ~IR_EMITTER_PORT;  // Toggle for 38kHz

        // Gesture Logic (v1.4, priority: detect even during other operations)
        if (gesture_state == GESTURE_IDLE) {
            if (r1_active && !r2_active && gesture_cooldown_counter == 0) {
                gesture_state = GESTURE_ARMED_R1;
                gesture_timeout_counter = 0;
            }
            if (r2_active && !r1_active && gesture_cooldown_counter == 0) {
                gesture_state = GESTURE_ARMED_R2;
                gesture_timeout_counter = 0;
            }
        }
        if (gesture_state == GESTURE_ARMED_R1) {  // Left to Right (power off)
            if (r2_active) {
                gesture_state = GESTURE_IDLE;
                gesture_cooldown_counter = GESTURE_COOLDOWN;
                if (k1_state == K1_STATE_FULLY_ON) {
                    k1_state = K1_STATE_SEQUENTIAL_OFF;
                    k1_sequential_index = (sizeof(sequentialOnLeds) / sizeof(LedCoord)) - 1;
                    k1_delay_counter = K1_SEQUENTIAL_DELAY;
                }
            }
            if (++gesture_timeout_counter > GESTURE_TIMEOUT) gesture_state = GESTURE_IDLE;
        }
        if (gesture_state == GESTURE_ARMED_R2) {  // Right to Left (power on)
            if (r1_active) {
                gesture_state = GESTURE_IDLE;
                gesture_cooldown_counter = GESTURE_COOLDOWN;
                if (k1_state == K1_STATE_OFF) k1_state = K1_STATE_INSTANT_ON;
            }
            if (++gesture_timeout_counter > GESTURE_TIMEOUT) gesture_state = GESTURE_IDLE;
        }
    }
}



void configure_io_pins(void) {
    _pcs15 = 0; _pcs14 = 0; _pcs17 = 0; _pcs16 = 0;
     SCLK_PORT_CTRL = 0; DIN_PORT_CTRL = 0;
    _pcs01 = 0; _pcs00 = 0; IR_EMITTER_CTRL = 0;
    _pcs05 = 0; _pcs04 = 0; _pcs07 = 0; _pcs06 = 0;
    _pcc2 = 1; _pcpu2 = 1; _pcc3 = 1; _pcpu3 = 1;
}

void init_STM(void) {
    _stmal = STM_PERIOD;
    _stmah = 0;
    _stmc1 = 0b11000001;  // Timer mode, clear on match A[1]
    _stmc0 = 0b00001100;  // fSYS clock, start
    _ston = 1;
}



// Display Functions (from v1.4, with precise_delay_us)
void precise_delay_us(unsigned int us) {
    volatile unsigned int i;
    for (i = 0; i < (us * 2); i++) { GCC_NOP(); }  // Calibrated for 8MHz
}
enum{
	encommand1=0,
	encommand2,
	enData,
	encommand3,
	enstart,
	enwrite,
	enendCommand,
	enstop,
	enComplete
};
unsigned char dataSet[20]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff},DisplayStaeNow=0;
void start(void) { SCLK_PORT = 1; DIN_PORT = 1; /*precise_delay_us(5);*/ DIN_PORT = 0; /*precise_delay_us(5);*/ SCLK_PORT = 0; /*precise_delay_us(5);*/ }
void stop(void) { SCLK_PORT = 0; DIN_PORT = 0; /*precise_delay_us(5);*/ SCLK_PORT = 1; /*precise_delay_us(5);*/ DIN_PORT = 1; /*precise_delay_us(5);*/ }
void writeByte(unsigned char data) { unsigned char i; for (i = 0; i < 8; i++) { SCLK_PORT = 0; /*precise_delay_us(5); */DIN_PORT = (data & 0x01); /*precise_delay_us(5);*/ SCLK_PORT = 1; /*precise_delay_us(5);*/ data >>= 1; }}
void sendCommand(unsigned char cmd) { start(); writeByte(cmd); stop(); }

void fvDisplayStateMachine(unsigned char command1,unsigned char command2, unsigned char *dataCommand,unsigned char command3, unsigned char datacount )
{
	static unsigned char i=0;
	
		
		switch(DisplayStaeNow)
		{
			case encommand1:
			    
			     SCLK_PORT = 1; 
			     DIN_PORT = 1;
			     SCLK_PORT = 0; 
			     DIN_PORT = 0;
			      writeByte(command1);
			      SCLK_PORT = 0; 
			     DIN_PORT = 0;
			     SCLK_PORT = 1; 
			     DIN_PORT = 1;
			      DisplayStaeNow = encommand2;
			      break;
			case encommand2:
			    
			      SCLK_PORT = 1; 
			     DIN_PORT = 1;
			     SCLK_PORT = 0; 
			     DIN_PORT = 0;
			      writeByte(command2);
			      SCLK_PORT = 0; 
			     DIN_PORT = 0;
			     SCLK_PORT = 1; 
			     DIN_PORT = 1;
			      DisplayStaeNow = enData;
			      break;
			 case enData:
			       for (i = 0; i < 16; i++)
			      {
			      writeByte(/**dataCommand*/0xff);
			    //  dataCommand++;
			      
			      }
			      DisplayStaeNow = encommand3;
			      break; 
			       
				case encommand3:
			   start();
			      writeByte(command3);
			      SCLK_PORT = 0; 
			     DIN_PORT = 0;
			     SCLK_PORT = 1; 
			     DIN_PORT = 1;
			      DisplayStaeNow = enstart;
			      break;
			 default:
			       break;
		}
	}
	


void updateDisplay(void) {
    unsigned char i;
    sendCommand(CMD_DATA_AUTO_INC);
   // start();
    writeByte(CMD_ADDRESS_SET);
    for (i = 0; i < 16; i++) writeByte(displayBuffer[i]);
   // stop();
    unsigned char cmd = 0x8f;// (brightness_level == 0) ? CMD_DISPLAY_ON_LOW : (brightness_level == 1) ? CMD_DISPLAY_ON_MED : CMD_DISPLAY_ON_HIGH;
    sendCommand(cmd);
   // fvDisplayStateMachine(CMD_DATA_AUTO_INC,CMD_ADDRESS_SET, &dataSet[0],0x8f, 16 );

}

void clearDisplay(void) {
    unsigned char i;
    for (i = 0; i < 16; i++) displayBuffer[i] = 0x00;
    updateDisplay();
}

void toggle_led(unsigned char seg, unsigned char grid) {
    if (seg >= 8 || grid >= 16) return;
    displayBuffer[grid] ^= (1 << seg);
}

void turn_off_led(unsigned char seg, unsigned char grid) {
    if (seg >= 8 || grid >= 16) return;
    displayBuffer[grid] &= ~(1 << seg);
}

void turn_on_led(unsigned char seg, unsigned char grid) {
    if (seg >= 8 || grid >= 16) return;
    displayBuffer[grid] |= (1 << seg);
}

// Power Sequences (exact from v1.4)
void process_k1_sequence(void) {
    unsigned char i;
    if (k1_state != K1_STATE_INSTANT_ON && k1_state != K1_STATE_SEQUENTIAL_ON) return;
    
    const unsigned char instant_led_count = sizeof(instantOnLeds) / sizeof(LedCoord);
    const unsigned char sequential_led_count = sizeof(sequentialOnLeds) / sizeof(LedCoord);
    const unsigned char exception_count = sizeof(exceptionLeds) / sizeof(LedCoord);
    switch (k1_state) {
        case K1_STATE_INSTANT_ON:
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
                    level = 0;
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
        } else {
            k1_sequential_index--;
        }
        updateDisplay();
    }
}

// 7-Segment Display (exact from v1.4)
void clear_custom_number_area(void) {
    unsigned char i;
    for (i = 0; i < 28; i++) {
        const LedCoord* led = all_seven_seg_leds[i];
        displayBuffer[led->grid] &= ~(1 << led->seg);
    }
}

void display_digit(unsigned char pos, unsigned char val) {
    unsigned char segment_mask;
    if (val == 255) return; // Blank
    if (val == 10) segment_mask = seven_seg_font[10]; // 'L'
    else segment_mask = seven_seg_font[val];
    unsigned char j;
    for (j = 0; j < 7; j++) {
        if ((segment_mask >> j) & 0x01) {
            const LedCoord* led_to_light = digit_place_map[pos][j];
            displayBuffer[led_to_light->grid] |= (1 << led_to_light->seg);
        }
    }
}

void display_seven_seg(void) {
    clear_custom_number_area();
    if (level == 0) {
        // Display "L0  "
        display_digit(0, 10); // 'L'
        display_digit(1, 0);  // '0'
        display_digit(2, 255); // Blank
        display_digit(3, 255); // Blank
    } else if (show_level) {
        // Display "Ln  " or "L10 "
        display_digit(0, 10); // 'L'
        if (level == 10) {
            display_digit(1, 1); // '1'
            display_digit(2, 0); // '0'
            display_digit(3, 255); // Blank
        } else {
            display_digit(1, level); // n
            display_digit(2, 255); // Blank
            display_digit(3, 255); // Blank
        }
    } else {
        // Display speed (e.g., 0500 for 500)
        unsigned int number = speeds[level];
        unsigned char digits[4];
        digits[0] = (number / 1000) % 10;
        digits[1] = (number / 100) % 10; 
        digits[2] = (number / 10) % 10;  
        digits[3] = number % 10;      
        unsigned char i;
        for (i = 0; i < 4; i++) {
            display_digit(i, digits[i]);
        }
    }
}

void process_touch_keys(void) {
    if(SCAN_CYCLEF){
        GET_KEY_BITMAP();
        // Swapped: Original K1 action now on K6 (power on/off sequence)
        if((DATA_BUF[0] & KEY_MASK_K6) && !key_flags[5]){
            key_flags[5] = 1;
            if (k1_state == K1_STATE_OFF) { k1_state = K1_STATE_INSTANT_ON; } 
            else if (k1_state == K1_STATE_FULLY_ON) { 
                k1_state = K1_STATE_SEQUENTIAL_OFF;
                k1_sequential_index = (sizeof(sequentialOnLeds) / sizeof(LedCoord)) - 1;
                k1_delay_counter = K1_SEQUENTIAL_DELAY;
            }
        } else if(!(DATA_BUF[0] & KEY_MASK_K6)){ key_flags[5]=0; }
        // Swapped: Original K2 action now on K5 (change brightness)
        if((DATA_BUF[0] & KEY_MASK_K5) && !key_flags[4] && k1_state == K1_STATE_FULLY_ON){
            key_flags[4]=1;
            brightness_level = (brightness_level + 1) % 3;
            switch (brightness_level) {
                case 0: sendCommand(CMD_DISPLAY_ON_LOW); break;
                case 1: sendCommand(CMD_DISPLAY_ON_MED); break;
                case 2: sendCommand(CMD_DISPLAY_ON_HIGH); break;
            }
        } else if(!(DATA_BUF[0] & KEY_MASK_K5)){ key_flags[4]=0; }
        // Swapped: Original K3 action now on K4 (increment level)
        if((DATA_BUF[0] & KEY_MASK_K4) && !key_flags[3] && k1_state == K1_STATE_FULLY_ON){
            key_flags[3]=1;
            if (boost) return;
            level = (level == 9) ? 1 : level + 1;
            if (level == 0) show_level = 1;
            else {
                show_level = 1;
                display_timer = 0;
            }
        } else if(!(DATA_BUF[0] & KEY_MASK_K4)){ key_flags[3]=0; }
        // Swapped: Original K4 action now on K3 (decrement level)
        if((DATA_BUF[2] & KEY_MASK_K3) && !key_flags[2] && k1_state == K1_STATE_FULLY_ON){
            key_flags[2]=1;
            if (boost) return;
            level = (level == 1) ? 9 : level - 1;
            if (level == 0) show_level = 1;
            else {
                show_level = 1;
                display_timer = 0;
            }
        } else if(!(DATA_BUF[2] & KEY_MASK_K3)){ key_flags[2]=0; }
        // Swapped: Original K5 action now on K2 (boost toggle)
        if((DATA_BUF[1] & KEY_MASK_K2) && !key_flags[1] && k1_state == K1_STATE_FULLY_ON){
            key_flags[1]=1;
            boost = !boost;
            if (boost) {
                previous_level = level;
                level = 10;
            } else {
                level = previous_level;
            }
            if (level == 0) show_level = 1;
            else {
                show_level = 1;
                display_timer = 0;
            }
        } else if(!(DATA_BUF[1] & KEY_MASK_K2)){ key_flags[1]=0; }
        // Swapped: Original K6 action now on K1 (toggle specific LED)
        if((DATA_BUF[2] & KEY_MASK_K1) && !key_flags[0] && k1_state == K1_STATE_FULLY_ON){
            key_flags[0]=1;
            toggle_led(5, 11);
        } else if(!(DATA_BUF[2] & KEY_MASK_K1)){ key_flags[0]=0; }
    }
}


void process_gesture_logic(void) {
    // Placeholder from v1.4 (gestures handled in ISR for priority)
}

// Main Initialization (exact from v1.4)
void USER_PROGRAM_C_INITIAL(void) {
    configure_system_clock();
    configure_io_pins();
    init_PTM_for_tick();
    Uart_Init();
   // fvSPI_iNIT();
     _tkm0c0 = (0b1001 << 4) | (0b000 << 3) | 1;
    _tkm0c1 = (0b00 << 6) | (0b00 << 2);
    _tkm1c0 = 0b00000111; _tkm1c1 = 0b00000000;
    _tkm2c0 = 0b00000010; _tkm2c1 = 0b00000000;
    _tkm3c0 = 0b00001100; _tkm3c1 = 0b00000000;
    _wdtc = 0b01010111;
    _ptmae = 1;
    _emi = 1;
  /*  u16TimermsDec = 5;
    while(u16TimermsDec)
    {
    	//uart_send_byte(5);
    }
    TM1640_send_command(auto_address);
    u16TimermsDec = 5;
    while(u16TimermsDec);
	uart_send_byte(_simd);
	TM1640_send_command(start_address);
	u16TimermsDec = 5;
    while(u16TimermsDec);
	uart_send_byte(_simd);
	char i=0;
	for( i=0;i<16;i++)
	{
		TM1640_send_command(0xFF);	
		
	}
	uart_send_byte(_simd);
	u16TimermsDec = 5;
    while(u16TimermsDec);
	TM1640_send_command(brightness_100_pc);  
	uart_send_byte(_simd); */
   // _pbc &= ~(1 << 4);
    
   
    
   
    
  //  init_STM();  // Added for gesture ISR
    
    //sendCommand(CMD_DISPLAY_ON_HIGH);
    //clearDisplay();
   // updateDisplay();
}

// Main Loop (exact from v1.4, with gesture priority via ISR)
void USER_PROGRAM_C(void) {
   /* if (app_tick_flag) {
        app_tick_flag = 0;
        
        if (gesture_cooldown_counter > 0) gesture_cooldown_counter--;
        
        process_gesture_logic();
        process_touch_keys();
        process_k1_sequence();
        process_k1_shutdown_sequence();
        
        if (k1_state == K1_STATE_FULLY_ON && show_level && level != 0) {
            if (++display_timer >= DISPLAY_DELAY_TICKS) {
                show_level = 0;
            }
        }
        
        if (k1_state == K1_STATE_FULLY_ON) {
            display_seven_seg();
        }
        
        updateDisplay();
    }*/
    if (u16Timerms>1000) {
        u16Timerms = 0; 
        uart_Send_String("IFB HOME APPLIANCE \n\r");
        
     /*   toggle_led(5, 11);
        	 u16TimermsDec = 5;
    while(u16TimermsDec);
    TM1640_send_command(auto_address);
    u16TimermsDec = 5;
    while(u16TimermsDec);
	uart_send_byte(_simd);
	TM1640_send_command(start_address);
	u16TimermsDec = 5;
    while(u16TimermsDec);
	uart_send_byte(_simd);
	char i=0;
	for( i=0;i<16;i++)
	{
		TM1640_send_command(0xFF);	
		
	}
	uart_send_byte(_simd);
	u16TimermsDec = 5;
     while(u16TimermsDec);
	TM1640_send_command(brightness_100_pc);  */
      
    }
     updateDisplay();
    GCC_CLRWDT();
}
