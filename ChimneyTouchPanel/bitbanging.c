/*** BS84D20CA – Turn-on-all-LEDs demo for TM1640 (Bit-Banged with Original Timings) ***/

#include "BS84D20CA.h"  // Built-in register definitions [1]

/* -------- Constants for TM1640 (or compatible) LED driver -------- */
#define CMD_DATA_AUTO_INC  0x40
#define CMD_ADDRESS_SET    0xC0
#define CMD_DISPLAY_ON_MAX 0x8F   /* display on, max brightness */

/* -------- Bit-bang pins (using built-in registers) -------- */
#define SCLK_PORT   _pc6    // PC6 as SCLK [2]
#define SCLK_PORT_CTRL _pcc6
#define DIN_PORT    _pc7    // PC7 as DIN [2]
#define DIN_PORT_CTRL _pcc7

/* -------- Delay copied from your code (~5 µs with 8 NOPs) -------- */
static void delay5us(void) {      /* ≈5 µs @8 MHz */
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
}

/* -------- Start/stop conditions (exact copy) -------- */
static void tm_start(void)
{
    SCLK_PORT = 1; DIN_PORT = 1; delay5us();
    DIN_PORT = 0; delay5us();
    SCLK_PORT = 0; delay5us();
}
static void tm_stop(void)
{
    SCLK_PORT = 0; DIN_PORT = 0; delay5us();
    SCLK_PORT = 1; delay5us();
    DIN_PORT = 1; delay5us();
}

/* -------- Send one byte, LSB first (exact copy, declaration moved) -------- */
static void tm_write_byte(unsigned char data)
{
    unsigned char i;  // Declaration moved here for non-C99
    for (i = 0; i < 8; i++) {
        SCLK_PORT = 0;           /* low */
        DIN_PORT = data & 0x01;  /* put bit */
        delay5us();
        SCLK_PORT = 1;           /* latch */
        delay5us();
        data >>= 1;
    }
}

/* -------- Minimal hardware init (using registers) -------- */
static void configure_pins(void)
{
    SCLK_PORT_CTRL = 0;      /* push-pull output [2] */
    DIN_PORT_CTRL  = 0;
}

/* -------- Public linker symbols (must exist) -------- */

void USER_PROGRAM_C_INITIAL(void)
{
    unsigned char i;  // Declaration moved here for non-C99

    configure_pins();

    /* power-up sequence for TM1640 */
    tm_start();                  /* auto-increment mode */
    tm_write_byte(CMD_DATA_AUTO_INC);
    tm_stop();

    tm_start();                  /* set address 0, then 16 bytes of 0xFF */
    tm_write_byte(CMD_ADDRESS_SET);
    for (i = 0; i < 16; i++) tm_write_byte(0xFF);
    tm_stop();

    tm_start();                  /* display on, max brightness */
    tm_write_byte(CMD_DISPLAY_ON_MAX);
    tm_stop();
}

/* never called, but must be present */
void USER_PROGRAM_C(void)            { while (1) { /* do nothing */ } }
void USER_PROGRAM_C_HALT_PREPARE(void){}
void USER_PROGRAM_C_HALT_WAKEUP(void) {}
void USER_PROGRAM_C_RETURN_MAIN(void){}
