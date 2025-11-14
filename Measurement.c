#include "mcc_generated_files/system/system.h"
#include "Measurement.h"
#include"DutyCycle.h"


 volatile uint8_t enParam_ADC,u8samples,u8InitDone,u8OverCurrentCount;
 volatile uint16_t u16ADC_RawValue[4][18],Movig_Avg_MosFetVolt[8];
 volatile uint16_t u16Current,Movig_Avg_Index;
 volatile uint16_t u8Volt,u8MOSFET_VOLT;
 volatile int16_t u8Temperature=1000;
 uint32_t u16Sum[4];
 void fvMeasurementProcess(void)
 {
     static uint8_t u8VoltCount=0,u8VoltCount_=0;
     if(u8samples>=17)
     {
        for(uint8_t j=0;j<4;j++)
        {
         u16Sum[j]=0;
         for(uint8_t i=1;i<17;i++)
         {
           u16Sum[j] += u16ADC_RawValue[j][i];
         }
        // u16Sum[j] /= u8samples;
     /*    if(j<3)
         {
              u16Sum[j] /= 1023.0; 
         }*/
        }
         u8MOSFET_VOLT = (((u16Sum[2]  )) * 1.8328);//+1100; 
         if(u8MOSFET_VOLT > 1)
            u8MOSFET_VOLT -= 1;
         //u8MOSFET_VOLT += 212;
         // u16Sum[2] = ((u16Sum[2] ) * 29.296875)+1500;
      //    u8MOSFET_VOLT = u8MOSFET_VOLT - ((1.0) * (u8MOSFET_VOLT - u16Sum[2]));
        //  u16Sum[1] = ((u16Sum[1] ) * 29.296875)+1500;
      //    u8Volt = (u16Sum[1] * 1.8328) ;
         u16Sum[1] *= 1.8328;
         u8Volt = u16Sum[1] + 550;//317;  //917
      //   if(u8Volt < u8MOSFET_VOLT)
      //     u8MOSFET_VOLT =  u8Volt -1; 
        //  u8Volt += 917;
        //  u8Volt = 13000;
       //   u16Current = (((u16Sum[0] / u8samples) / 1024.0) * 5.0) / 0.03 ;//Amps into 10
           u16Sum[0] *= 93;//68;//102  ;
        //  if(u16Sum[3] >= 510)
         //u16Current=(((u16Sum[3]*1.48)-748.96));//Amps into 10
         // u16Sum[0] += 700;
            u16Current = u16Sum[0];
         //u16Current=1500;
         //u8Temperature = u8Temperature - ((0.05) * (u8Temperature - u16Sum[3]));
          
         u8Temperature = u16Sum[3] >> 4;
        //  u8Temperature = 100;
         if(u8Volt < 7000 || u8Volt >/*16000*/18000)
         {
             u8VoltCount++;
             if(u8VoltCount> 100)
             {
              u8VoltCount = 0;
         //     EnErrorState |= VOLT_ERROR;
              
             }
         }
           else 
         {
             u8VoltCount=0;
          //   EnErrorState = EnErrorState & (~VOLT_ERROR);
         }
       if(EnErrorState & VOLT_ERROR)
            {
                if(u8Volt > 7500 || u8Volt < /*16500*/18500)
                {
                u8VoltCount_++;
                if(u8VoltCount_ > 100)
                {
                u8VoltCount_ = 0;
                EnErrorState &= ~VOLT_ERROR;
                }
                }
                else
                    u8VoltCount_=0;
            }
        
      
        
         u8MeasurementDone=1;
         u8samples=0;
       //    enParam_ADC=0;
      //   ADC0_StartConversion(enParam_ADC);
     }
 }