#include "System_Clock.h"
#include "BS84D20CA.h"

void configure_system_clock(void) {
    _hirc1 = 0; _hirc0 = 0; // 8MHz
    _hircen = 1;
    while(!_hircf);
    _scc = 0b00000000;
}
