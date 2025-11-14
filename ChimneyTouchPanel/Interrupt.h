#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "BS84D20CA.h"

typedef struct { unsigned char seg; unsigned char grid; } LedCoord;

extern volatile unsigned char app_tick_flag ;
extern volatile unsigned char heartbeat_flag ;
extern volatile unsigned char blink_flag; // New flag for blinking
extern volatile unsigned char blink_flag1;
extern unsigned char k1_delay_counter;
extern unsigned char k1_sequential_index;
extern volatile unsigned char gesture_state;
extern volatile unsigned char k1_state;
extern volatile unsigned int RXTimeout;

extern const LedCoord instantOnLeds[12];
extern const LedCoord sequentialOnLeds[25];
extern const LedCoord exceptionLeds[60];
// Enums

enum K1State { K1_STATE_OFF, K1_STATE_INSTANT_ON, K1_STATE_SEQUENTIAL_ON, K1_STATE_FULLY_ON, K1_STATE_SEQUENTIAL_OFF };

// Constants (assuming defined elsewhere or include relevant headers)
#define IR_RECEIVER_1       _pc3
#define IR_RECEIVER_2       _pc2
#define K1_SEQUENTIAL_DELAY 1
#define TICKS_PER_APP_TICK  39
#define TICKS_PER_HEARTBEAT 49

// Function Prototypes
void Multi_Function_ISR(void);
void PTM_Timer_ISR(void);
void USER_PROGRAM_C_HALT_PREPARE(void);
void USER_PROGRAM_C_HALT_WAKEUP(void);
void USER_PROGRAM_C_RETURN_MAIN(void);

#endif
