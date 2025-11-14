#include "PeriodicTimer.h"

volatile unsigned long u16Timerms;


// PTM ISR (exact from v1.4)
void __attribute__((interrupt(0x1C))) PTM_Timer_ISR(void) {
    static unsigned long app_tick_counter = 0;
    static unsigned long heartbeat_counter = 0;
    if (_ptmaf) _ptmaf = 0;
    _pbc = ~_pbc;
   /* if (++app_tick_counter >= TICKS_PER_APP_TICK) {
        app_tick_counter = 0;
        app_tick_flag = 1;
    }
    if (++heartbeat_counter >= TICKS_PER_HEARTBEAT) {
        heartbeat_counter = 0;
        heartbeat_flag = 1;
    }*/
    u16Timerms++;
}

void init_PTM_for_tick(void) {
    _ptmc0 = 0b00100000;  // fSYS/16 (500khz)
    _ptmc1 = 0b11000001;  // Timer mode, clear on CCRA
    _ptmal = (500 - 1) & 0xFF;
    _ptmah = (500 - 1) >> 8;
    _pton = 1;
}
