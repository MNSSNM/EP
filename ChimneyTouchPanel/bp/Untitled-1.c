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

void fvSPI_iNIT(void)
{
	// Enable SIM for shifting (LSB first, SCK idle high, change on falling)
	 // Initial pin config
    _pcs14 = 1; _pcs15 = 1;  // PC6=SCK (pin-shared) [5]
    _pcs16 = 1; _pcs17 = 1;  // PC7=SDO (pin-shared) [5]

    _sim2 = 0; _sim1 = 0; _sim0 = 1;  // Master, fSYS/4 [6]
    _ckpolb = 1;  // SCK idle high [6]
    _ckeg = 1;    // Change on falling, capture rising [6]
    _mls = 0;     // LSB first [6]
    _csen = 0;    // No CS (TM1640 has none) [6]
    _simen = 1;   // Enable SIM [6]

}
void tm_write_byte(unsigned char data) {
  
     
    _simd = data;  // Load and transmit [6]
    while (_trf==0); // Wait complete [6]
    _trf = 0;      // Clear flag [6]

}

/* Initialization */
void USER_PROGRAM_C_INITIAL(void) {
	unsigned char i;
   _pcs1 = 0b10010000;
   fvSPI_iNIT();
    // Power-up sequence
    // _csen = 0;
  while(1)
  {
     DELAY_5US();
     DELAY_5US();
     DELAY_5US();
    tm_write_byte(CMD_DATA_AUTO_INC);
     DELAY_5US();
     DELAY_5US();
     DELAY_5US();
     DELAY_5US();
     DELAY_5US();
     DELAY_5US();

    tm_write_byte(CMD_ADDRESS_SET);
    for (i = 0; i < 16; i++)
    {
     tm_write_byte(0xFF);  // All on

  //  
    }
    tm_write_byte(CMD_DISPLAY_ON_MAX);
  }
   //  _csen = 1;
}

void USER_PROGRAM_C(void) { while (1) { /* Idle */ } }
void USER_PROGRAM_C_HALT_PREPARE(void) {}
void USER_PROGRAM_C_HALT_WAKEUP(void) {}
void USER_PROGRAM_C_RETURN_MAIN(void) {}
