#include "Uart.h"
#include "BS84D20CA.h"
#include "Led_Driver.h"

unsigned char tbyte[20];
unsigned char rxbyte[20],DataLen;
unsigned char switchstatus;
unsigned int checksum, RPM,ErrorCode,ReadRPM;
volatile unsigned char tready;
unsigned int ErrorCode;


void __attribute((interrupt(0x28))) uart(void)
{
	if((_usr & 0x04) == 0x04)
	{
	   rxbyte[DataLen++]= _txr_rxr; 
	   RXTimeout = 20;
	 // _txr_rxr = rxbyte[0];
	}
}


void init_uart(void) {
    // --- Baud Rate Calculation (8MHz clock for 4800 baud) ---
    // Formula: Baud Rate = fH / (BRD + UMOD/8)
    // Target: 8,000,000 / 4800 = 1666.666...
    // BRD = TRUNC(1666.666) = 1666 (0x0682)
    // UMOD = ROUND(0.666 * 8) = ROUND(5.328) = 5 (0b101)
    
    _brdl = 0x41; // Baud rate divider low byte
    _brdh = 0x03; // Baud rate divider high byte
    
    // Set modulation bits for accuracy and BRD range
    _ufcr = 0b00011001; // UMOD[2:0]=101, BRDS=0

    // --- UART Control Registers ---
    // Enable UART, 8-bit data, no parity
    _ucr1 = 0b10000000; // UARTEN=1, BNO=0, PREN=0
    
    // Enable Transmitter and Receiver, 1 stop bit for receiver
    _ucr2 = 0b11000100; // TXEN=1, RXEN=1, STOPS=0
    
    // Disable Single Wire Mode (use separate TX/RX pins)
    _ucr3 = 0b00000000; // SWM=0
    
    _ure = 1;
}

void uart_send_byte(unsigned char data) {
    // Wait for the transmit buffer to be empty by polling the TXIF flag.
    while ((_usr & 0x01) == 0); // Wait for TXIF (bit 0) to be 1

    // Load the data into the transmit/receive register to send it.
    _txr_rxr = data;
}

/*void UartRxDataProcess(void)
{

	if(DataLen >= 8)
	{
		DataLen = 0;
		if(rxbyte[0] == 1)
		{
			 fvchecksumcalc((char*)&rxbyte[0],7);
			 if( (rxbyte[7] == (char)checksum) && (rxbyte[8] == (char)(checksum >> 8)) || 1)
			 {
			 	rxbyte[0] = 0;
			 	ErrorCode = 1;
			 	ErrorCode = rxbyte[1];
			 	ErrorCode |= (rxbyte[2] << 8);
			 	ReadRPM = 0;
			 	ReadRPM = rxbyte[3];
			 	ReadRPM |= (rxbyte[4] << 8);
			    //tready=1;
			 }
		}
	}
}*/

void UartRxDataProcess(void)
{
	// 1. Check if a full 9-byte packet has been received
	if(DataLen >= 1 && RXTimeout==0)
	{
		// Reset the counter for the next packet
		DataLen = 0;
		
		// 2. Check if the packet starts with the correct header byte (1)
		if(rxbyte[0] == 2)
		{
			 // 3. Calculate the checksum of the data payload (first 7 bytes)
			 fvchecksumcalc((unsigned char*)&rxbyte[0], 7);
			 
			 // 4. CORRECTED: Compare checksums using (unsigned char) to prevent overflow
			 if( (rxbyte[7] == (unsigned char)checksum) && (rxbyte[8] == (unsigned char)(checksum >> 8)) )
			 {
			 	// --- CHECKUM VALID: Process the data ---
			 	
			 	ErrorCode = 0;
			 	
			 	// Mark the packet as processed by clearing its header
			 	rxbyte[0] = 0;
			 	
			 	// 5. Reconstruct the 16-bit ErrorCode from the low and high bytes
			 	ErrorCode = rxbyte[1];         // Low byte
			 	ErrorCode |= (rxbyte[2] << 8); // High byte
			 	
			 	// Pass the error code to the handler function
			 //	handle_critical_error(ErrorCode);

			 	// 6. Reconstruct the 16-bit RPM value from its low and high bytes
			 	ReadRPM = 0; // Clear previous value
			 	ReadRPM = rxbyte[3];         // Low byte
			 	ReadRPM |= (rxbyte[4] << 8); // High byte
			 }
		}
	}
}

void uart_t_byte(void) {
	
	static unsigned char i=0;
	
	if(tready==1)
	{
	
		if( i==0 )
		
		{
		
		    tbyte[0]=1;
		    tbyte[1]=switchstatus;
		    tbyte[2]=0;
		    
		    RPM = speeds[level];
		    
		    tbyte[3]=(char)RPM;
		    tbyte[4]=(char)(RPM>>8);
		    tbyte[5]=0;
			tbyte[6]=0;
		    
		    fvchecksumcalc((char*)&tbyte[0],7);
		    
		    tbyte[7]=(char)checksum;
		    tbyte[8]=(char)(checksum>>8);
	    
		}
	    
	    // Wait for the transmit buffer to be empty by polling the TXIF flag.
	   if((_usr & 0x01) == 1)
	    {
	    	_txr_rxr = (char)tbyte[i];
	    	
	    	i++;
	    	
	    	if(i>8)
	    	{	
	    		i=0;
	    		tready=0;
	    		_pdc6 = 1;
	    		_pds1 |= 0b00100000; 
	    		
	    	}	
	
	    } // Wait for TXIF (bit 0) to be 1

	}
    // Load the data into the transmit/receive register to send it.
}

void fvchecksumcalc(unsigned char *Data, char Len)
{	unsigned char i=0;
    unsigned char *uData = (unsigned char *)Data; // Cast to unsigned
	checksum=0;
	
	for(i=0;i<Len;i++)
	{
		checksum += *uData; // Use the unsigned pointer
		uData++;
	}
}


void uartSS(unsigned char *String_) {
    while(*String_ != 0) {
        while ((_usr & 0x01) == 0);
        _txr_rxr = *String_;
        String_++;
    }
}

/*unsigned char uart_receive_byte(void) {
    // Wait for the receive buffer to have data by polling the RXIF flag.
    while ((_usr & 0x02) == 0); // Wait for RXIF (bit 1) to be 1
    // Read the data from the transmit/receive register.
    return _txr_rxr;
}

void uart_receive_string(unsigned char *buffer, unsigned int max_length) {  
    unsigned int i = 0;
    while (i < max_length - 1) {
        unsigned char byte = uart_receive_byte();
        if (byte == '\0') break; // Stop if null terminator is received
        buffer[i++] = byte;
    }
    buffer[i] = '\0'; // Null-terminate the string
}*/


