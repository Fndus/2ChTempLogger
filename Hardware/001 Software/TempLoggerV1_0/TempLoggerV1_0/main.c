/*
 * TempLoggerV1_0.c
 *
 * Created: 02.02.2024 22:04:33
 * Author : Fndus
 */ 

#define F_CPU 3333333										// 20MHz
#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 *(float)BAUD_RATE)) + 0.5)
// eingebundene Bibliotheken
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <string.h>
#include <math.h> 
// Variablen

static volatile uint8_t receiveData = 0;
static volatile uint8_t writeData = 0;
static volatile uint8_t value = 0;




void USART0_init(void);
void USART0_putc(char c);
void USART0_puts(char *str);
char USART0_getc(void);
void UserCommands(char *command);
void init_SPI(void);
void Write_CH1(unsigned char w_addr,unsigned char data);
unsigned char Read_CH1(unsigned char r_addr);
void Write_CH2(unsigned char w_addr,unsigned char data);
unsigned char Read_CH2(unsigned char r_addr);
void Read_temp_Ch1(void);
void Read_temp_Ch2(void);
void ftoa(float n, char* res, int afterpoint);
int intToStr(int x, char str[], int d);
void reverse(char* str, int len);

int main(void)
{
	//char s[7];
	//int16_t i = -12345;
	//uint8_t a;
	//a = USART0_getc();
	//USART0_puts(itoa(i,s,10));							// i = intvariable, s=string, 10 = Dezimalsystem
	//USART0_putc(a);
	//_delay_ms(1000);
	
	USART0_init();
	init_SPI();
	
	char command[50];										// Befehl max  zeichen
	uint8_t index = 0;
	char c;
	//_delay_ms(1000);
    while (1)												// zeichenkette bis 0x0D einlesen und befehl ausfueren 
    {	
		c = USART0_getc();
		if(c != 0x0D && c != 0x0A)
		{
			command[index++] = c;
			if(index > strlen(command))
			{
				index = 0;
			}
		}
		
		if(c == 0x0D)
		{
			command[index] = '\0';
			index = 0;
			UserCommands(command);
		}
	}
		
}

void USART0_init(void)
{
	PORTMUX.CTRLB |= 0x05;									// alternative Pinbelegung für usart und spi
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600);			// set baud rate
	
	PORTA.DIR &= ~PIN2_bm;									// PA2 RXD INPUT
	PORTA.DIR |= PIN1_bm;									// PA1 TXD OUTPUT

	USART0.CTRLB |= USART_TXEN_bm	| USART_RXEN_bm;;		// enable transmitter + receiver
}

void USART0_putc(char c)
{
	while (!(USART0.STATUS & USART_DREIF_bm))				// warten bis senden moeglich
	{
		;
	}
	USART0.TXDATAL = c;										// zeichen auf schnittstelle schreiben
}

void USART0_puts(char *str)
{
	for(size_t i = 0; i < strlen(str); i++)
	{
		USART0_putc(str[i]);
	}
}

char USART0_getc(void)
{
	while (!(USART0.STATUS & USART_RXCIF_bm))				// warten bis daten verfuegbar
	{
		;
	}
	return USART0.RXDATAL;
}

void init_SPI(void)
{	PORTMUX.CTRLB |= 0x05;										// Rout SPi to PortB
	SPI0.CTRLA |= SPI_MASTER_bm;									// select master mode
	SPI0.CTRLA |= SPI_PRESC_DIV16_gc;								// CLK_PER = 3333333 / 16
	SPI0.CTRLA |= SPI_CLK2X_bm;									// enable double clock speed
	SPI0.CTRLB |= SPI_SSD_bm;										//
	SPI0.CTRLB |= SPI_MODE_3_gc;
	//SPI0.CTRLA |= SPI_DORD_bm;									// LSB first
	SPI0.CTRLA |= SPI_ENABLE_bm;									// enable spi
	
	PORTC.DIRCLR = PIN1_bm;											// MISO channel input
	//PORTC.PIN1CTRL = PORT_PULLUPEN_bm;
	PORTC.DIRSET = PIN2_bm;											// MOSI channel output
	PORTC.DIRSET = PIN0_bm;											// SCK channel output
	PORTC.DIRSET = PIN3_bm;											// SS0 channel output
	PORTB.DIRSET = PIN1_bm;											// SS1 output
	
	PORTC.OUTSET = PIN3_bm;											// ss0 high
	PORTB.OUTSET = PIN1_bm;											// ss1 high
	
	SPI0.DATA;														// empty spi		Write_CH1(0x80,0x80);
	Write_CH1(0x81,0x43);
	Write_CH2(0x80,0x80);
	Write_CH2(0x81,0x43);}

void Write_CH1(unsigned char w_addr,unsigned char data)
{	PORTC.OUTCLR = PIN3_bm;	SPI0.DATA = w_addr;
	while (!(SPI0.INTFLAGS & SPI_IF_bm)) 	{			;	}	SPI0.DATA = data;	while (!(SPI0.INTFLAGS & SPI_IF_bm))	{		;	}	PORTC.OUTSET = PIN3_bm;}

unsigned char Read_CH1(unsigned char r_addr)
{	PORTC.OUTCLR = PIN3_bm;	SPI0.DATA = r_addr;
	while (!(SPI0.INTFLAGS & SPI_IF_bm))	{		;	}	SPI0.DATA = 0xFF;
	while (!(SPI0.INTFLAGS & SPI_IF_bm))	{		;	}	value = SPI0.DATA;
	//USART0_sendChar(value);	PORTC.OUTSET = PIN3_bm;	return value;}void Write_CH2(unsigned char w_addr,unsigned char data)
{	PORTB.OUTCLR = PIN1_bm;	SPI0.DATA = w_addr;
	while (!(SPI0.INTFLAGS & SPI_IF_bm))	{		;	}	SPI0.DATA = data;
	while (!(SPI0.INTFLAGS & SPI_IF_bm))	{		;	}	PORTB.OUTSET = PIN1_bm;}unsigned char Read_CH2(unsigned char r_addr)
{	PORTB.OUTCLR = PIN1_bm;	SPI0.DATA = r_addr;
	while (!(SPI0.INTFLAGS & SPI_IF_bm))	{		;	}	SPI0.DATA = 0xFF;
	while (!(SPI0.INTFLAGS & SPI_IF_bm))	{		;	}	value = SPI0.DATA;
	//USART0_sendChar(value);	PORTB.OUTSET = PIN1_bm;	return value;}

void Read_temp_Ch1(void){	char s[20];	double temp = 0;	float t1 =0;	t1 = Read_CH1(0x0C);
	temp = t1*16;
	t1 = Read_CH1(0x0D);
	temp = temp + t1*0.06225586;
	t1 = Read_CH1(0x0E);
	temp = temp+ t1/4095;
	ftoa(temp,s,1);
	USART0_puts(s);	USART0_puts("\r\n");}void Read_temp_Ch2(void){	char s[20];	double temp = 0;	float t1 =0;	t1 = Read_CH2(0x0C);
	temp = t1*16;
	t1 = Read_CH2(0x0D);
	temp = temp + t1*0.06225586;
	t1 = Read_CH2(0x0E);
	temp = temp+ t1/4095;
	ftoa(temp,s,1);
	USART0_puts(s);	USART0_puts("\r\n");}

void reverse(char* str, int len)
{
	int i = 0, j = len - 1, temp;
	while (i < j) {
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++;
		j--;
	}
}

int intToStr(int x, char str[], int d)
{
	int i = 0;
	while (x) {
		str[i++] = (x % 10) + '0';
		x = x / 10;
	}
	
	// If number of digits required is more, then
	// add 0s at the beginning
	while (i < d)
	str[i++] = '0';
	
	reverse(str, i);
	str[i] = '\0';
	return i;
}

void ftoa(float n, char* res, int afterpoint)
{
	// Extract integer part
	int ipart = (int)n;
	
	// Extract floating part
	float fpart = n - (float)ipart;
	
	// convert integer part to string
	int i = intToStr(ipart, res, 0);
	
	// check for display option after point
	if (afterpoint != 0) {
		res[i] = '.'; // add dot
		
		// Get the value of fraction part upto given no.
		// of points after dot. The third parameter
		// is needed to handle cases like 233.007
		fpart = fpart * pow(10, afterpoint);
		
		intToStr((int)fpart, res + i + 1, afterpoint);
	}
}

void UserCommands(char *command)
{
	if(strcmp(command, "ON") == 0)
	{
		USART0_puts("OK, LED ON.\r\n");
	}
	else if(strcmp(command, "TempCh1") == 0)
	{
		Read_temp_Ch1();
	}
	else if(strcmp(command, "TempCh2") == 0)
	{
		Read_temp_Ch2();
	}
	else 
    {
        USART0_puts("Incorrect command.\r\n");
    }
}