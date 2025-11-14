/* 
 * File:   DutyCycle.h
 * Author: Dynamometer
 *
 * Created on 9 October, 2023, 8:36 PM
 */

#ifndef DUTYCYCLE_H
#define	DUTYCYCLE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "DataType.h"
    
//#define _CAPTURE_TO_DUTY 100.0/( 12500.0 * 4.0)
    
    extern volatile uint16_t u16RawCaptureCount[10],u16RawFreqCount[6],PowerSupplyStableTimer,TargetVolt_Count,MotorVolt_count,DAC_Count;
    extern volatile uint8_t u8CaptureLen,u8CaptureTimeOut;  // 8
    extern uint8_t u8MeasurementDone;
    
    typedef enum enMotorState5 {
        MOTOR_STOP,
        MOTOR_RUN,
    }EnMotorState1;
    extern EnMotorState1 EnMotorState;
    typedef enum enInverterFault{
	NO_ERROR,
	VOLT_ERROR=1,
	TEMP_ERROR=2,
	OVER_CURR_ERROR=4,
	}EnErrorState1;
extern EnErrorState1 EnErrorState;
void fvDutyCycleCalc(void);

#ifdef	__cplusplus
}
#endif

#endif	/* DUTYCYCLE_H */

