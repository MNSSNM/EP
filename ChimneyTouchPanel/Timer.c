#include "Timer.h"
#include "BS84D20CA.h"

// STM Initialization
void init_CTM(void) {
	_ctmc0 = 0b00000000;  // fSYS clock, start
	_ctmc1 = 0b11000001;  // Timer mode, clear on match A[1]
    _ctmal = (1024 - 1) & 0xFF;
    _ctmah = (1024 - 1) >> 8;
    _cton = 1;
}

void init_STM(void) {
    // Set CCRA to 210 for ~38 kHz (fSYS=8 MHz, clock=fSYS, period=210 cycles)
    _stmal = 0x69;
    _stmah = 0x00;

    // Set STMC1: Compare Match Output mode (00), toggle output (11), initial low, non-inverted, STCCLR=1 (clear on match A)
    _stmc1 = 0b00110001;

    // Set STMC0: clock = fSYS (001), start off
    _stmc0 = 0b00010000;

    // Start the STM
    _ston = 1;
}

// PTM Initialization for Tick
void init_PTM_for_tick(void) {
    _ptmc0 = 0b00000000;  // fSYS/4 (2MHz)
    _ptmc1 = 0b11000001;  // Timer mode, clear on CCRA
    _ptmal = (1024 - 1) & 0xFF;
    _ptmah = (1024 - 1) >> 8;
    _pton = 1;
}
