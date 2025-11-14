

#include "mcc_generated_files/system/system.h"
#include "Measurement.h"
#include"DutyCycle.h"
/*
    Main application
*/
/*Updated to 35Amps, temperature compensation 145 deg*/
int main(void)
{
    SYSTEM_Initialize();
     u8InitDone = 1;
     ADC0_ChannelSelect(enParam_ADC);
     ADC0_ConversionStart();
     PowerSupplyStableTimer=10;
     while(PowerSupplyStableTimer);
    /* Replace with your application code */
    while (1){
       
       fvMeasurementProcess();
       fvDutyCycleCalc();
       
    }
}