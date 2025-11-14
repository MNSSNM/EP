/*** BS84D20CA Hybrid SIM + GPIO Driver for TM1640 ***/
/*** Uses SIM registers for byte shift, GPIO for start/stop ***/

#include "BS84D20CA.h"  // Register definitions [3]

/* TM1640 Commands */
#define CMD_DATA_AUTO_INC  0x40
#define CMD_ADDRESS_SET    0xC0
#define CMD_DISPLAY_ON_MAX 0x8F

/* Pins: PC6=SCLK, PC7=SDO (DIN) [5] */
#define SCLK _pc6
#define DIN  _pc7
#define SCLK_CTRL _pcc6
#define DIN_CTRL  _pcc7

/* Delay ~5µs (40 cycles at 8MHz) */
#define DELAY_5US() GCC_DELAY(40)

/* Hybrid write byte: GPIO start, SIM shift, GPIO stop */
void tm_write_byte(unsigned char data) {
    // Start: Disable SIM, use GPIO
    _simen = 0;  // Disable SIM [6]
    SCLK_CTRL = 0; DIN_CTRL = 0;  // Outputs [5]
    SCLK = 1; DIN = 1; DELAY_5US();
    DIN = 0; DELAY_5US();
    SCLK = 0; DELAY_5US();

    // Enable SIM for shifting (LSB first, SCK idle high, change on falling)
    _sim2 = 0; _sim1 = 0; _sim0 = 1;  // Master, fSYS/4 [6]
    _ckpolb = 0;  // SCK idle high [6]
    _ckeg = 0;    // Change on falling, capture rising [6]
    _mls = 0;     // LSB first [6]
    _csen = 0;    // No CS (TM1640 has none) [6]
    _simen = 1;   // Enable SIM [6]

    // Shift byte using built-in register
    _simd = data;  // Load and transmit [6]
    while (!_trf); // Wait complete [6]
    _trf = 0;      // Clear flag [6]

    // Stop: Disable SIM, use GPIO
    _simen = 0;    // Disable SIM [6]
    SCLK_CTRL = 0; DIN_CTRL = 0;  // Outputs
    SCLK = 0; DIN = 0; DELAY_5US();
    SCLK = 1; DELAY_5US();
    DIN = 1; DELAY_5US();
}

/* Initialization */
void USER_PROGRAM_C_INITIAL(void) {
    // Initial pin config
    _pcs14 = 1; _pcs15 = 1;  // PC6=SCK (pin-shared) [5]
    _pcs16 = 1; _pcs17 = 1;  // PC7=SDO (pin-shared) [5]

    // Power-up sequence
    tm_write_byte(CMD_DATA_AUTO_INC);

    tm_write_byte(CMD_ADDRESS_SET);
    for (unsigned char i = 0; i < 16; i++) tm_write_byte(0xFF);  // All on

    tm_write_byte(CMD_DISPLAY_ON_MAX);
}

void USER_PROGRAM_C(void) { while (1) { /* Idle */ } }
void USER_PROGRAM_C_HALT_PREPARE(void) {}
void USER_PROGRAM_C_HALT_WAKEUP(void) {}
void USER_PROGRAM_C_RETURN_MAIN(void) {}
