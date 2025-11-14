#ifndef __LED_DRIVER_H__
#define __LED_DRIVER_H__

#include "BS84D20CA.h"
#include "Interrupt.h"

// Constants
#define SCLK_PORT           _pc6
#define SCLK_PORT_CTRL      _pcc6
#define DIN_PORT            _pc7
#define DIN_PORT_CTRL       _pcc7
#define CMD_DATA_AUTO_INC   0x40
#define CMD_ADDRESS_SET     0xC0
#define CMD_DISPLAY_ON_LOW  0x88
#define CMD_DISPLAY_ON_MED  0x8A
#define CMD_DISPLAY_ON_HIGH 0x8F
#define DISPLAY_DELAY_TICKS 1

// Struct
//typedef struct { unsigned char seg; unsigned char grid; } LedCoord;

// LED Coordinate Definitions
extern unsigned char displayBuffer[16];
extern unsigned char brightness_level;
extern unsigned char level;
extern unsigned char boost;
extern unsigned char previous_level;
extern unsigned char display_timer;
extern unsigned char show_level;
extern const unsigned int speeds[];
extern unsigned char error_code;

//const unsigned char LedCoord instantOnLeds[6];
//const unsigned char LedCoord sequentialOnLeds[25];
//const unsigned char LedCoord exceptionLeds[29];

// Function Prototypes
void start(void);
void stop(void);
void writeByte(unsigned char data);
void sendCommand(unsigned char cmd);
void updateDisplay(void);
void clearDisplay(void);
void toggle_led(unsigned char seg, unsigned char grid);
void turn_off_led(unsigned char seg, unsigned char grid);
void turn_on_led(unsigned char seg, unsigned char grid);
void clear_custom_number_area(void);
void display_digit(unsigned char pos, unsigned char val);
void display_seven_seg(void);

void display_standby_pattern(void);
extern const LedCoord standbyLeds[];

#endif
