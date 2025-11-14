/************
 * Title: Simple Swipe Detection with 38kHz Output for BS84D20CA
 * Description: Uses STM for 38kHz on PC0 and detects swipes on PC3 (left)/PC2 (right, active high).
 *              Left->Right: LEDs 1,2,4; Right->Left: LEDs 6,3,5 (from matrix).
 * Date: August 5, 2025
 ************/
#include "BS84D20CA.h"
#include "USER_PROGRAM_C.INC"
// Pin Configuration
#define IR_EMITTER_PORT     _pc0
#define IR_EMITTER_CTRL     _pcc0
#define IR_RECEIVER_1       _pc3  // Left (PC3, active high)
#define IR_RECEIVER_2       _pc2  // Right (PC2, active high)
// Constants
#define STM_PERIOD          26     // For ~38kHz toggle (fSYS=8MHz)
#define SWIPE_TIMEOUT_TICKS 100000  // ~1 second at 38kHz ISR rate (adjust for precision)
// Gesture States
enum { GESTURE_IDLE, GESTURE_ARMED_LEFT, GESTURE_ARMED_RIGHT };
// Globals
unsigned char displayBuffer[16] = {0};
volatile unsigned char gesture_state = GESTURE_IDLE;
volatile unsigned int gesture_timeout_counter = 0;
// Forward Declarations
void configure_system_clock(void);
void configure_io_pins(void);
void init_STM(void);
void init_PTM(void);
void updateDisplay(void);
void turn_on_led(unsigned char seg, unsigned char grid);
void timer_delay_us(unsigned int us);
// Missing handlers (to fix linker errors)
void USER_PROGRAM_C_HALT_PREPARE(void) {}
void USER_PROGRAM_C_HALT_WAKEUP(void) {}
void USER_PROGRAM_C_RETURN_MAIN(void) {}
// STM ISR: Toggle PC0 for 38kHz and handle swipe detection with 1s timeout
void __attribute__((interrupt(0x34))) Multi_Function_ISR(void) {
    unsigned char r1_active = IR_RECEIVER_1;  // Left active (high)
    unsigned char r2_active = IR_RECEIVER_2;  // Right active (high)
    if (_stmaf) {
        _stmaf = 0;
        IR_EMITTER_PORT = ~IR_EMITTER_PORT;  // Toggle for 38kHz
        // Gesture Logic with timeout
        if (gesture_state == GESTURE_IDLE) {
            if (r1_active && !r2_active) {
                gesture_state = GESTURE_ARMED_LEFT;
                gesture_timeout_counter = 0;
            }
            if (r2_active && !r1_active) {
                gesture_state = GESTURE_ARMED_RIGHT;
                gesture_timeout_counter = 0;
            }
        }
        if (gesture_state == GESTURE_ARMED_LEFT) {
            if (r2_active) {  // Left to Right completed
                // Turn on some LEDs (from matrix: 1,2,4)
                turn_on_led(2, 2);  // Position 1
                gesture_state = GESTURE_IDLE;
            }
            if (++gesture_timeout_counter > SWIPE_TIMEOUT_TICKS) {
                gesture_state = GESTURE_IDLE;  // Timeout: reset
            }
        }
        if (gesture_state == GESTURE_ARMED_RIGHT) {
            if (r1_active) {  // Right to Left completed
                // Turn on some other LEDs (from matrix: 6,3,5)
                turn_on_led(3, 3);  // Position 6
                gesture_state = GESTURE_IDLE;
            }
            if (++gesture_timeout_counter > SWIPE_TIMEOUT_TICKS) {
                gesture_state = GESTURE_IDLE;  // Timeout: reset
            }
        }
    }
}
// System Initialization
void configure_system_clock(void) {
    _hirc1 = 0; _hirc0 = 1; // 8MHz
    _hircen = 1;
    while(!_hircf);
    _scc = 0b00000000;
}
void configure_io_pins(void) {
    IR_EMITTER_CTRL = 0;  // PC0 output
    _pcc3 = 1; _pcpu3 = 1;  // PC3 input with pull-up
    _pcc2 = 1; _pcpu2 = 1;  // PC2 input with pull-up
    // Display pins (minimal, assume PC6 SCLK, PC7 DIN)
    _pcc6 = 0;  // SCLK output
    _pcc7 = 0;  // DIN output
}
void init_STM(void) {
    _stmal = STM_PERIOD;
    _stmah = 0;
    _stmc1 = 0b11000001;  // Compare Match A, Clear Counter, Timer Mode
    _stmc0 = 0b00001100;  // Clock = fSYS, Timer initially STOPPED
    _ston = 1;  // Start STM
}
void init_PTM(void) {
    _ptmc1 = 0b11000000;  // Timer mode (ptm1 ptm0=11), ptcclr=1 (clear on CCRA, but not used for polling)
    _ptmc0 = 0b00000000;  // Clock fSYS/4 (000), pton=0 initially
}
void timer_delay_us(unsigned int us) {
    unsigned int ticks = 2 * us;  // PTM clock fSYS/4 = 2MHz (assuming fSYS=8MHz), tick=0.5us, so 2 ticks per us

    // Clear PTM counter
    _ptmdl = 0;
    _ptmdh = 0;

    // Start PTM
    _pton = 1;

    // Poll until counter >= ticks
    while (1) {
        unsigned int current = (_ptmdh << 8) | _ptmdl;
        if (current >= ticks) break;
    }

    // Stop PTM
    _pton = 0;
}
void start(void) { _pc6 = 1; _pc7 = 1; timer_delay_us(5); _pc7 = 0; timer_delay_us(5); _pc6 = 0; timer_delay_us(5); }
void stop(void) { _pc6 = 0; _pc7 = 0; timer_delay_us(5); _pc6 = 1; timer_delay_us(5); _pc7 = 1; timer_delay_us(5); }
void writeByte(unsigned char data) { unsigned char i; for (i = 0; i < 8; i++) { _pc6 = 0; timer_delay_us(5); if (data & 0x01) { _pc7 = 1; } else { _pc7 = 0; } timer_delay_us(5); _pc6 = 1; timer_delay_us(5); data >>= 1; } }
void sendCommand(unsigned char cmd) { start(); writeByte(cmd); stop(); }
void updateDisplay(void) {
    unsigned char i;
    sendCommand(0x40);  // Data auto-increment
    start();
    writeByte(0xC0);    // Address set to 0x00
    for (i = 0; i < 16; i++) {
        writeByte(displayBuffer[i]);
    }
    stop();
    sendCommand(0x8F);  // Display on, max brightness
}
void turn_on_led(unsigned char seg, unsigned char grid) {
    if (seg < 8 && grid < 16) displayBuffer[grid] |= (1 << seg);
    updateDisplay();
}
// Main Initialization
void USER_PROGRAM_C_INITIAL(void) {
    configure_system_clock();
    configure_io_pins();
    init_STM();
    init_PTM();  // Initialize PTM for timer-based delays
    // Enable interrupts
    _stmae = 1;  // STM Interrupt Enable
    _mfe = 1;    // Multi-Function Interrupt Enable
    _emi = 1;    // Global Interrupt Enable
}
// Main Loop
void USER_PROGRAM_C(void) {
    GCC_CLRWDT();
}
