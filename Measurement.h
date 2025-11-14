/* 
 * File:   Measurement.h
 * Author: Dynamometer
 *
 * Created on November 27, 2023, 1:32 PM
 */

#ifndef MEASUREMENT_H
#define	MEASUREMENT_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "DataType.h"
    
  extern volatile uint8_t enParam_ADC,u8samples,u8InitDone,u8OverCurrentCount;
 extern  volatile uint16_t u16ADC_RawValue[4][18];
 extern volatile uint16_t u8Volt, u8MOSFET_VOLT,u16Current;
 extern volatile int16_t u8Temperature;
  void fvMeasurementProcess(void);


#ifdef	__cplusplus
}
#endif

#endif	/* MEASUREMENT_H */

