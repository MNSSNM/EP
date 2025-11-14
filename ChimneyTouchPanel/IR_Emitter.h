#ifndef __IR_EMITTER_H__
#define __IR_EMITTER_H__

#include "BS84D20CA.h"


// Pin Definitions
#define IR_EMITTER_PORT     _pc0
#define IR_EMITTER_CTRL     _pcc0

// Countdown constants (app tick every 25ms)
#define COUNTDOWN_SECONDS 60
#define TICKS_PER_SECOND (1000 / 25)  // 40 ticks per second
#define COUNTDOWN_TOTAL_TICKS (COUNTDOWN_SECONDS * TICKS_PER_SECOND)  // 4000 ticks for 100s

enum GestureState { GESTURE_IDLE, GESTURE_ARMED_R1, GESTURE_ARMED_R2 };

// Gesture states (expanded for robustness)
#define GESTURE_IDLE 0
#define GESTURE_ARMED_LEFT_START 1  // Left receiver triggered first
#define GESTURE_ARMED_RIGHT_START 2  // Right receiver triggered first
#define GESTURE_COMPLETED_LEFT_TO_RIGHT 3  // Full left to right swipe
#define GESTURE_COMPLETED_RIGHT_TO_LEFT 4  // Full right to left swipe
#define GESTURE_TIMEOUT 100  // Adjust based on swipe speed (in app ticks)
#define GESTURE_COOLDOWN 20  // Cooldown after gesture to prevent rapid triggers

// New globals for gesture changes and countdown
extern unsigned char off_sequence_state ;  // 0: normal, 1: in countdown, 2: at l0
extern unsigned int countdown_ticks_remaining ;  // Countdown tick counter
extern unsigned char is_gesture_poweron ;
extern volatile unsigned int countdown_tick_counter ;  // For per-second updates

// <<< START OF CHANGES >>>
// Gesture state variables declared as extern
extern volatile unsigned char gesture_state;
extern volatile unsigned int gesture_timeout_counter;
extern volatile unsigned int gesture_cooldown_counter;

extern unsigned char hstatus;

// Function Prototypes for gesture actions
void perform_left_to_right_action(void);
void perform_right_to_left_action(void);
// <<< END OF CHANGES >>>


// Function Prototypes
void initialize_gesture(void);

#endif