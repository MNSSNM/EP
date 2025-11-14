#ifndef __PeriodicTimer___
#define __PeriodicTimer___

#include "BS84D20CA.h"


// --- Function Prototypes ---
void init_PTM_for_tick(void);

extern volatile unsigned long u16Timerms,u16TimermsDec;
#endif