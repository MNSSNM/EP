#ifndef __UART_H__
#define __UART_H__

#include "BS84D20CA.h"

extern volatile unsigned char tready;
extern unsigned char switchstatus;
extern unsigned int ErrorCode;

// Function Prototypes
void configure_uart_pins(void);
void init_uart(void);
void uart_send_byte(unsigned char data);
void uartSS(unsigned char *String_);
void Uart_Init(void);
unsigned char uart_receive_byte(void);
void uart_receive_string(unsigned char *buffer, unsigned int max_length);
void uart_t_byte(void);
void fvchecksumcalc(unsigned char *Data, char Len);
void UartRxDataProcess(void);


#endif
