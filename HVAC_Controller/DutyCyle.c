#include "mcc_generated_files/system/system.h"
#include"DutyCycle.h"
#include "Measurement.h"
volatile uint16_t u16RawCaptureCount[10],u16RawFreqCount[6],PowerSupplyStableTimer,DAC_Count,MotorVolt,TargetVolt,TargetVolt_Count,MotorVolt_count,Volt_Diff_Motor_Target;
volatile uint8_t u8CaptureLen,u8CaptureTimeOut;  // 8
uint8_t u8MeasurementDone,u8Temp_Compensation;
EnMotorState1 EnMotorState;
EnErrorState1 EnErrorState;
int16_t s16Target_v;
uint16_t u8DutyCycle;
uint16_t  u16CurrentLimit=3000,u16CurrentLimit_Temp=35000,u16TempratureThreshold=133;
int32_t Temp_Calc=0;
void fvDutyCycleCalc(void)
{
    static uint8_t u8PresentMotorState=255,u8PastMotorState=255,u8TemperatureCount=0,u8TemperatureCount1=0;
     
    if(u8CaptureTimeOut==0)
    {
        u8DutyCycle = 0;
        u8CaptureLen=0;
    }
   
    else if(u8CaptureLen >= 4)
    {
        for(uint8_t i=1;i<4;i++)
       {
           u16RawCaptureCount[0] += u16RawCaptureCount[i];
      //     u16RawFreqCount[0] += u16RawFreqCount[i];
       }
     //   u8DutyCycle = ((u16RawCaptureCount[0] * 100.0) /  u16RawFreqCount[0]);
          u16RawCaptureCount[0] = u16RawCaptureCount[0] >> 2;
          u8DutyCycle = (u16RawCaptureCount[0] * 0.0125);//0.003125;//0.0025/*_CAPTURE_TO_DUTY*/;
          if(u8DutyCycle > 50)
               u8DutyCycle -= 1;   
     //   u16Freq = 2000000.0 / u16RawFreqCount[0];
     //   u8DutyCycle = 95;
        u8CaptureLen=0;
        u16RawCaptureCount[0]=0;
       // u16RawCaptureCount[0] = 0;
        TCB0_CAPTInterruptEnable();
    }
   
    if(u8DutyCycle<11 || u8DutyCycle >= 98 || EnErrorState != NO_ERROR)
        {
        
            EnMotorState = MOTOR_STOP;
          //  u16CurrentLimit = 5000;
            DAC0_SetOutput(0);
            DAC_Count=0;
             if(PowerSupplyStableTimer==0)
        {
            if(EnErrorState & OVER_CURR_ERROR)
            {
                EnErrorState &= ~OVER_CURR_ERROR;
            }
            
           
        }
        }
        else 
        {
            
       
            if(EnErrorState == NO_ERROR && u8MeasurementDone==1 && PowerSupplyStableTimer==0)
            {
             /*Temp Compensation Off*/
                 /*   MotorVoltTransfer Func  --> (DC / 100) * 2.8 * 5 
                  * wE CONVERT MILLIVOLT * 1000 SO 140*/
                     
               
                 if(u8Temperature >= /*133*/u16TempratureThreshold)
                {  
                    u8TemperatureCount=0;
                    if(u8DutyCycle < 90)
                    TargetVolt = (140 * u8DutyCycle); 
                    else
                         TargetVolt = 20000; 
                  //  if(u8DutyCycle >= 90)
                     u16CurrentLimit = (MotorVolt * 2.2);
                      u16CurrentLimit += 5000;
                      u16CurrentLimit_Temp = 35000 ;
                  u16TempratureThreshold = 133;
                }
                 else{
                     
                    u8TemperatureCount++;
                    if(u8TemperatureCount >= 100)
                    {
                     //   u16CurrentLimit = (int32_t)20 * u8Temperature + 16223;  maintain 19 amps
                       // u16CurrentLimit = (int32_t)775 * u8Temperature - 90362; working 15.5vdc 22 amps.
                        Temp_Calc = (int32_t)889 * u8Temperature - 92333;
                        if(Temp_Calc < 5000)
                        {
                            u16CurrentLimit_Temp = 2000;
                            TargetVolt = 1000;
                        }
                        else
                            u16CurrentLimit_Temp = Temp_Calc;
                           u8TemperatureCount = 0;
                           u16TempratureThreshold = 167;
                          // TargetVolt=0;
                      }
              }
                
                if(u16CurrentLimit > u16CurrentLimit_Temp)
                     u16CurrentLimit = u16CurrentLimit_Temp;
                
                if(u8Volt >= u8MOSFET_VOLT) {
                MotorVolt = u8Volt - u8MOSFET_VOLT;
                }      
               //  u16CurrentLimit = (int32_t)167 * u8Temperature - 29666;
               //  u16CurrentLimit = (int32_t)150 * u8Temperature - 24000;
                
                if (u16CurrentLimit > 35000)
                    u16CurrentLimit = 35000;
            
              /*  else  if(EnMotorState == MOTOR_STOP)
                {  
                  MotorVolt =  2000; 
                }*/
                if(MotorVolt>=TargetVolt)
                    Volt_Diff_Motor_Target = MotorVolt-TargetVolt;
                else
                   Volt_Diff_Motor_Target = TargetVolt-MotorVolt; 
                
                if(u16CurrentLimit < u16Current)
              {
                  if(DAC_Count)
                      DAC_Count--;
                //  PowerSupplyStableTimer=500;
              }
                
                else  if(Volt_Diff_Motor_Target<10)
                {
                    MotorVolt=TargetVolt;
                }
                
                else if(MotorVolt>TargetVolt /*|| u16CurrentLimit < u16Current*/)
              {
                  if(DAC_Count)
                      DAC_Count--;
                //  PowerSupplyStableTimer=500;
              }
       
              /*    else if(MotorVolt==(TargetVolt-0))
                  {
                      MotorVolt=TargetVolt;
                  }*/
                else if(MotorVolt<TargetVolt)  
              {
                  DAC_Count++;
                  u8PresentMotorState = EnMotorState;
                  if(u8PresentMotorState != u8PastMotorState)
                  {
                     u8PastMotorState = u8PresentMotorState;
                     DAC_Count = 125;
                  }/**/
                  if(DAC_Count>255)
                    DAC_Count=255;
                
              }
                
                if(TargetVolt==0 /*|| u8Temperature <= 180*/)
                    DAC_Count=0;
               DAC0_SetOutput(DAC_Count); 
               u8MeasurementDone=0;
               EnMotorState = MOTOR_RUN;
               //PowerSupplyStableTimer=1000;
            }
            
        /*    else if(EnErrorState != NO_ERROR)
            {
                DAC0_SetOutput(0);
                DAC_Count=0;
                EnMotorState = MOTOR_STOP;
            }*/
        }
}
