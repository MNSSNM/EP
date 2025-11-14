/************
 * Title: Simple Swipe Detection with 38kHz Output for BS84D20CA
 * Description: Uses STM for 38kHz on PC0 and detects swipes on PC3 (left)/PC2 (right, active high).
 *              Left->Right: Sequential LED off; Right->Left: Sequential LED on.
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

// Added from v1.4: Tick constants (PTM base tick ~0.512ms at 2MHz clock)
#define TICKS_PER_APP_TICK  39  // ~20ms (39 * 0.512 ≈ 19.968ms)
#define TICKS_PER_HEARTBEAT 49  // ~25ms (49 * 0.512 ≈ 25.088ms)

// Added from v1.4: Gesture constants
#define GESTURE_TIMEOUT     15
#define GESTURE_COOLDOWN    25

// Gesture States (updated to v1.4 enum)
enum { GESTURE_IDLE, GESTURE_ARMED_R1, GESTURE_ARMED_R2 };

// Added from v1.4 for sequential LED control: Struct and array
typedef struct { unsigned char seg; unsigned char grid; } LedCoord;
const LedCoord sequentialOnLeds[] = { {5, 14}, {4, 14}, {4, 6}, {1, 6}, {2, 6}, {3, 6}, {0, 6}, {4, 7}, {5, 7}, {4, 1}, {5, 1}, {4, 10}, {5, 10}, {3, 5}, {1, 5}, {2, 5}, {4, 5}, {0, 5}, {5, 5}, {0, 2}, {3, 2}, {2, 2}, {1, 2}, {4, 2}, {5, 13} };

// Globals
unsigned char displayBuffer[16] = {0};
volatile unsigned char gesture_state = GESTURE_IDLE;
volatile unsigned int gesture_timeout_counter = 0;

// Added from v1.4: Tick flags
volatile unsigned char app_tick_flag = 0;
volatile unsigned char heartbeat_flag = 0;

// Added from v1.4: Gesture cooldown
unsigned char gesture_cooldown_counter = 0;

// Added for sequential control
unsigned char k1_sequential_index = 0;

// Forward Declarations
void configure_system_clock(void);
void configure_io_pins(void);
void init_STM(void);
void init_PTM_for_tick(void);  // From Step 1
void updateDisplay(void);
void turn_on_led(unsigned char seg, unsigned char grid);
void timer_delay_us(unsigned int us);
void process_gesture_logic(void);  // Added stub from v1.4

// Missing handlers (to fix linker errors)
void USER_PROGRAM_C_HALT_PREPARE(void) {}
void USER_PROGRAM_C_HALT_WAKEUP(void) {}
void USER_PROGRAM_C_RETURN_MAIN(void) {}

// STM ISR: Toggle PC0 for 38kHz and enhanced swipe detection with sequential LED on/off
void __attribute__((interrupt(0x34))) Multi_Function_ISR(void) {
    unsigned char r1_active = IR_RECEIVER_1;  // Left active (high)
    unsigned char r2_active = IR_RECEIVER_2;  // Right active (high)
    if (_stmaf) {
        _stmaf = 0;
        IR_EMITTER_PORT = ~IR_EMITTER_PORT;  // Toggle for 38kHz

        // Gesture Logic with timeout and cooldown (from v1.4)
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
        if (gesture_state == GESTURE_ARMED_R1) {  // Left to Right: Sequential LED off
            if (r2_active) {
                // Sequential turn off
                if (k1_sequential_index < sizeof(sequentialOnLeds) / sizeof(LedCoord)) {
                    displayBuffer[sequentialOnLeds[k1_sequential_index].grid] &= ~(1 << sequentialOnLeds[k1_sequential_index].seg);
                    k1_sequential_index++;
                } else {
                    k1_sequential_index = 0;  // Reset after full sequence
                }
                updateDisplay();  // Reflect change
                gesture_state = GESTURE_IDLE;
                gesture_cooldown_counter = GESTURE_COOLDOWN;
            }
            if (++gesture_timeout_counter > GESTURE_TIMEOUT) {
                gesture_state = GESTURE_IDLE;  // Timeout: reset
            }
        }
        if (gesture_state == GESTURE_ARMED_R2) {  // Right to Left: Sequential LED on
            if (r1_active) {
                // Sequential turn on
                if (k1_sequential_index < sizeof(sequentialOnLeds) / sizeof(LedCoord)) {
                    displayBuffer[sequentialOnLeds[k1_sequential_index].grid] |= (1 << sequentialOnLeds[k1_sequential_index].seg);
                    k1_sequential_index++;
                } else {
                    k1_sequential_index = 0;  // Reset after full sequence
                }
                updateDisplay();  // Reflect change
                gesture_state = GESTURE_IDLE;
                gesture_cooldown_counter = GESTURE_COOLDOWN;
            }
            if (++gesture_timeout_counter > GESTURE_TIMEOUT) {
                gesture_state = GESTURE_IDLE;  // Timeout: reset
            }
        }
    }
}

// Added from v1.4: PTM Timer ISR to create app tick (~20ms) and heartbeat tick (~25ms)
void __attribute__((interrupt(0x1C))) PTM_Timer_ISR(void) {
    static unsigned char app_tick_counter = 0;
    static unsigned char heartbeat_counter = 0;
    if (_ptmaf) _ptmaf = 0;

    // App tick (~20ms)
    if (++app_tick_counter >= TICKS_PER_APP_TICK) {
        app_tick_counter = 0;
        app_tick_flag = 1;
    }

    // Heartbeat tick (~25ms)
    if (++heartbeat_counter >= TICKS_PER_HEARTBEAT) {
        heartbeat_counter = 0;
        heartbeat_flag = 1;
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

// From Step 1: Initialize PTM timer for tick with 0.512ms period (~2MHz clock)
void init_PTM_for_tick(void) {
    _ptmc0 = 0b00000000;  // PTCK=000 (fSYS/4=2MHz), PTON=0
    _ptmc1 = 0b11000001;  // Mode=11 (Timer), PTCCLR=1 (clear on CCRA)
    _ptmal = (1024 - 1) & 0xFF;  // 1023 for ~0.512ms interrupt
    _ptmah = (1024 - 1) >> 8;
    _pton = 1;  // Start
}
void timer_delay_us(unsigned int us) {

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

// Added from v1.4: Stub for gesture logic (called in main loop)
void process_gesture_logic(void) {
    // For now, placeholder as per v1.4; can expand if needed
}

// Main Initialization
void USER_PROGRAM_C_INITIAL(void) {
    configure_system_clock();
    configure_io_pins();
    init_STM();
    init_PTM_for_tick();  // From Step 1
    // Enable interrupts
    _stmae = 1;  // STM Interrupt Enable
    _ptmae = 1;  // PTM Interrupt Enable (from Step 1)
    _mfe = 1;    // Multi-Function Interrupt Enable
    _emi = 1;    // Global Interrupt Enable
}

// Main Loop (updated with cooldown decrement in app tick)
void USER_PROGRAM_C(void) {
    if (app_tick_flag) {
        app_tick_flag = 0;
        // Added from v1.4: Decrement cooldown on tick
        if (gesture_cooldown_counter > 0) gesture_cooldown_counter--;
        process_gesture_logic();  // Added call
        // Placeholder for other tick-based processing
    }

    if (heartbeat_flag) {
        heartbeat_flag = 0;
        // Placeholder for heartbeat processing
    }

    GCC_CLRWDT();
}
