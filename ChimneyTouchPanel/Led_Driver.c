#include "LED_Driver.h"
#include "BS84D20CA.h"
#include "Interrupt.h"
#include "Uart.h"


// Modify this line. Change the size from [11] to [12] and add the new pattern at the end.
const unsigned char seven_seg_font[12] = { 0b0111111, 0b0000110, 0b1011011, 0b1001111, 0b1100110, 0b1101101, 0b1111101, 0b0000111, 0b1111111, 0b1101111, 0b0111000, 0b1111001 }; // Added 'E' at index 11 // Added index 10 for 'L' (0b0111000)

// Arrays (defined with sizes for sizeof visibility)
const LedCoord instantOnLeds[12] = { {5, 4}, {4, 4}, {0, 4}, {2, 4}, {1, 4}, {3, 4},{0, 3}, {1, 3}, {3, 3}, {4, 3}, {5, 3},};
//const LedCoord sequentialOnLeds[25] = { {5, 14}, {4, 14}, {4, 6}, {1, 6}, {2, 6}, {3, 6}, {0, 6}, {4, 7}, {5, 7}, {4, 1}, {5, 1}, {4, 10}, {5, 10}, {3, 5}, {1, 5}, {2, 5}, {4, 5}, {0, 5}, {5, 5}, {0, 2}, {3, 2}, {2, 2}, {1, 2}, {4, 2}, {5, 13} };
//const LedCoord exceptionLeds[29] = { {3, 14}, {3, 10}, {3, 1}, {3, 7}, {3, 13}, {3, 12}, {3, 11}, {1, 14}, {1, 10}, {1, 1}, {1, 7}, {1, 13}, {1, 12}, {1, 11}, {2, 14}, {2, 10}, {2, 1}, {2, 7}, {2, 13}, {2, 12}, {2, 11}, {0, 14}, {0, 10}, {0, 1}, {0, 7}, {0, 13}, {0, 12}, {0, 11}, {5, 11} };
const LedCoord standbyLeds[] = { {0, 3}, {1, 3}, {3, 3}, {4, 3}, {5, 3},
								 {0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4} };

// 7-Segment Mappings (as in original)
const LedCoord d1_sA = {1, 14}; const LedCoord d1_sB = {1, 13}; const LedCoord d1_sC = {1, 1};  const LedCoord d1_sD = {1, 10}; const LedCoord d1_sE = {1, 7};  const LedCoord d1_sF = {1, 12}; const LedCoord d1_sG = {1, 11};
const LedCoord d2_sA = {2, 14}; const LedCoord d2_sB = {2, 13}; const LedCoord d2_sC = {2, 1};  const LedCoord d2_sD = {2, 10}; const LedCoord d2_sE = {2, 7};  const LedCoord d2_sF = {2, 12}; const LedCoord d2_sG = {2, 11};
const LedCoord d3_sA = {3, 14}; const LedCoord d3_sB = {3, 13}; const LedCoord d3_sC = {3, 1};  const LedCoord d3_sD = {3, 10}; const LedCoord d3_sE = {3, 7};  const LedCoord d3_sF = {3, 12}; const LedCoord d3_sG = {3, 11};
const LedCoord d4_sA = {0, 14}; const LedCoord d4_sB = {0, 13}; const LedCoord d4_sC = {0, 1};  const LedCoord d4_sD = {0, 10}; const LedCoord d4_sE = {0, 7};  const LedCoord d4_sF = {0, 12}; const LedCoord d4_sG = {0, 11};
const LedCoord* const all_seven_seg_leds[28] = {
    &d1_sA, &d1_sB, &d1_sC, &d1_sD, &d1_sE, &d1_sF, &d1_sG, &d2_sA, &d2_sB, &d2_sC, &d2_sD, &d2_sE, &d2_sF, &d2_sG,
    &d3_sA, &d3_sB, &d3_sC, &d3_sD, &d3_sE, &d3_sF, &d3_sG, &d4_sA, &d4_sB, &d4_sC, &d4_sD, &d4_sE, &d4_sF, &d4_sG
};
const LedCoord* const digit_a_map[7] = { &d1_sA, &d1_sB, &d1_sC, &d1_sD, &d1_sE, &d1_sF, &d1_sG };
const LedCoord* const digit_b_map[7] = { &d2_sA, &d2_sB, &d2_sC, &d2_sD, &d2_sE, &d2_sF, &d2_sG };
const LedCoord* const digit_c_map[7] = { &d3_sA, &d3_sB, &d3_sC, &d3_sD, &d3_sE, &d3_sF, &d3_sG };
const LedCoord* const digit_d_map[7] = { &d4_sA, &d4_sB, &d4_sC, &d4_sD, &d4_sE, &d4_sF, &d4_sG };
const LedCoord* const* const digit_place_map[4] = {
    digit_a_map, // Position 0
    digit_b_map, // Position 1
    digit_c_map, // Position 2
    digit_d_map  // Position 3
};

// Display Functions
void start(void) { SCLK_PORT = 1; DIN_PORT = 1; DIN_PORT = 0; SCLK_PORT = 0; }
void stop(void) { SCLK_PORT = 0; DIN_PORT = 0; SCLK_PORT = 1; DIN_PORT = 1; }
void writeByte(unsigned char data) { unsigned char i; for (i = 0; i < 8; i++) { SCLK_PORT = 0; DIN_PORT = (data & 0x01); SCLK_PORT = 1; data >>= 1; } }
void sendCommand(unsigned char cmd) { start(); writeByte(cmd); stop(); }

void updateDisplay(void) {
    unsigned char i;
    sendCommand(CMD_DATA_AUTO_INC);
    start();
    writeByte(CMD_ADDRESS_SET);
    for (i = 0; i < 16; i++) writeByte(displayBuffer[i]);
    stop();
    unsigned char cmd = (brightness_level == 0) ? CMD_DISPLAY_ON_LOW : (brightness_level == 1) ? CMD_DISPLAY_ON_MED : CMD_DISPLAY_ON_HIGH;
    sendCommand(cmd);
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

// 7-Segment Display Functions
void clear_custom_number_area(void) {
    unsigned char i;
    for (i = 0; i < 28; i++) {
        const LedCoord* led = all_seven_seg_leds[i];
        displayBuffer[led->grid] &= ~(1 << led->seg);
    }
}

// Modify the display_digit function
void display_digit(unsigned char pos, unsigned char val) {
    unsigned char segment_mask;
    if (val == 255) return; // Blank
    
    // Add the 'else if' condition for 'E'
    if (val == 10) segment_mask = seven_seg_font[10]; // 'L'
    else if (val == 11) segment_mask = seven_seg_font[11]; // 'E'
    else segment_mask = seven_seg_font[val];

    unsigned char j;
    for (j = 0; j < 7; j++) {
        if ((segment_mask >> j) & 0x01) {
            const LedCoord* led_to_light = digit_place_map[pos][j];
            displayBuffer[led_to_light->grid] |= (1 << led_to_light->seg);
        }
    }
}

// Modify the display_seven_seg function
void display_seven_seg(void) {
    clear_custom_number_area();
	
    // Add this entire 'if' block at the very beginning of the function
    if (ErrorCode > 0) {
            int detected_error_code=0;

    // Parse the errorCode according to the bitmask
    if (ErrorCode & 1)      detected_error_code = 1; // E1
    else if (ErrorCode & 2) detected_error_code = 2; // E2
    else if (ErrorCode & 4) detected_error_code = 3; // E3
    else if (ErrorCode & 8) detected_error_code = 4; // E4
    else if (ErrorCode & 16) detected_error_code = 5; // E5
    else if (ErrorCode & 32) detected_error_code = 6; // E6
    else if (ErrorCode & 64) detected_error_code = 7; // E7
    else if (ErrorCode & 128) detected_error_code = 8; // E8
    // This can be extended for all 16 bits if needed

    // If a valid error was found, lock the system
    //if (detected_error_code > 0) {
    //    error_code = detected_error_code;

        // --- CRITICAL ERROR LOCKDOWN ---
        // The program will be trapped in this loop until the power is cycled.
       /* while(1) {
            display_seven_seg(); // This will now exclusively show the error
            updateDisplay();
            GCC_CLRWDT(); // Reset the watchdog timer to prevent a system reset
        }*/
        
        display_digit(0, 255); // Blank digit 'a'
        display_digit(1, 11);  // 'L' on digit 'b'
        display_digit(2, detected_error_code);   // '0' on digit 'c'
        display_digit(3, 255); // Blank digit 'd'
    //}
    } 
    else if (level == 0) { // The rest of the function is the same
        // Display " L0 "
        display_digit(0, 255); // Blank digit 'a'
        display_digit(1, 10);  // 'L' on digit 'b'
        display_digit(2, 0);   // '0' on digit 'c'
        display_digit(3, 255); // Blank digit 'd'
    } else if (show_level) {
        if (level >= 1 && level <= 9) {
            // Display " Ln " for levels 1-9
            display_digit(0, 255);   // Blank digit 'a'
            display_digit(1, 10);    // 'L' on digit 'b'
            display_digit(2, level); // 'n' on digit 'c'
            display_digit(3, 255);   // Blank digit 'd'
        } else if (level == 10) {
            // Keep original display for L10 as it requires 3 digits
            display_digit(0, 10); // 'L'
            display_digit(1, 1);  // '1'
            display_digit(2, 0);  // '0'
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

void display_standby_pattern(void) {
    unsigned char i;
    
	brightness_level = 1;
    for (i = 0; i < 16; i++) {
        displayBuffer[i] = 0x00;
    }

    // Turn on the LEDs for the standby pattern
    const unsigned char standby_led_count = sizeof(standbyLeds) / sizeof(LedCoord);
    for (i = 0; i < standby_led_count; i++) {
        turn_on_led(standbyLeds[i].seg, standbyLeds[i].grid);
    }
    
    // Send the changes to the display driver
    updateDisplay();
}
