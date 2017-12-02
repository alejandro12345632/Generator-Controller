/*
 * CS122A_FINAL_PROJECT.c
 *
 * Created: 11/29/2017 9:32:15 AM
 * Author : LovePoki
 */ 

#include <avr/io.h>
#include "usart_ATmega1284.h"
#include <avr/interrupt.h>
#include "lcd.h"
#include "bit.h"
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
float x;
unsigned char button1 = 0x00;
unsigned char button2 = 0x00;
unsigned char button3 = 0x00;
unsigned char button4 = 0x00;
char TimeHolder;
 char btemp;
 char Bluetooth_Status_Slctr =0x00;						//holds Bluetooth Input from phone '1' is on '0'; default '2'
 char OFF_Bluetooth_Status_Slctr =0x04;					//USED to trigger off after Engine was turned on
unsigned char SetBit1(unsigned char x, unsigned char k, unsigned char b) {
	return (b ?  (x | (0x01 << k))  :  (x & ~(0x01 << k)) );
}

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}



void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

// Initializes the desired USART.
// Call this function before the while loop in main.
void initUSART(unsigned char usartNum);

// Empties the UDR register of the desired USART, this will cause USART_HasReceived to return false.
void USART_Flush(unsigned char usartNum);

// Returns a non-zero number if the desired USART is ready to send data.x
// Returns 0 if the desired USART is NOT ready to send data.
unsigned char USART_IsSendReady(unsigned char usartNum);

// Returns a non-zero number if the desired USART has finished sending data.
// Returns 0 if the desired USART is NOT finished sending data.
unsigned char USART_HasTransmitted(unsigned char usartNum);

// Returns a non-zero number if the desired USART has received a byte of data.
// Returns 0 if the desired USART has NOT received a byte of data.
unsigned char USART_HasReceived(unsigned char usartNum);

// Writes a byte of data to the desired USARTs UDR register.
// The data is then sent serially over the TXD pin of the desired USART.
// Call this function after USART_IsSendReady returns 1.
void USART_Send(unsigned char data, unsigned char usartNum);

// Returns the data received on RXD pin of the desired USART.
// Call this function after USART_HasReceived returns 1.
unsigned char USART_Receive(unsigned char usartNum);

/*
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
}
*/
enum States {idle, Geni_press,Geni_release,Sht_Dwn_press,Sht_Dwn_rlse,Sensor_Polling,Auto_Strt_press,Auto_Srt_release,Auto_Time_Slct_Prss,Auto_P1_press,Auto_P2_press,Auto_P1_rlse,Auto_P2_rlse,BlueTooth_ON_Prss,BlueTooth_ON_Rlse}state;	
	
void Tick()
{
	button1 = ~PIND & 0x01;
	button2 = ~PIND & 0x02;
	button3 = ~PIND & 0x10;
	button4 = ~PIND & 0x20;
	
	switch(state)
	{
		case idle:								//In idle
		if(button1 ) 
		{	
			state = Geni_press;					//Manual Start
		}
		
		
		else if(!button3 && !button1 && button2)
		{
			state = Auto_Strt_press;
		}
		
		else if(!button3 && !button2 && button4 && !button1)
		{
			state = BlueTooth_ON_Prss;
		}
		
		else
		{
			state = idle;
		}
		break;
		
		case BlueTooth_ON_Prss:
		if(!button1&&!button2&&!button3&& button4)
		{
			state = BlueTooth_ON_Prss;
		}
		
		else if(!button1&&!button2&&!button3&& !button4)
		{
			state = BlueTooth_ON_Rlse;
		}
		break;
		
		case BlueTooth_ON_Rlse:
		if(Bluetooth_Status_Slctr !=0x01)						//keep polling until Bluetooth_Status_Slctr != '2';
		{
			ReceiveBluetooth();
			state = BlueTooth_ON_Rlse;
		}
		
		
		if(Bluetooth_Status_Slctr == 0x01)						//Selected to Start through Bluetooth
		{
			LCD_DisplayString(1,"Bluetooth:1 ON");
			delay_ms(1500);
			state = Geni_release;
			
		}
		
		
		break;
		
		
		
		
		case Geni_press:
			if(button1 ) {
				state = Geni_press;
			}

		
			else if(!button1 )
			{
				state = Geni_release;
			}

		break;
		
		case Geni_release:						//Begins to poll ADC data
		{
			state = Sensor_Polling;					
		}
		break;
		
		case Sensor_Polling:					//infintie Sensor loop; FIX
			
			
		if(button3&&!button1&&!button2)
		{
			state = Sht_Dwn_press;
		}
		
		
		else
		{
			state = Sensor_Polling;
		}
		
		break;
		
		case Auto_Strt_press:
		if(button2 && !button1 && !button3)
		{
			state = Auto_Strt_press;
		}
		
		else if(!button2 && !button1 && !button3)
		{
			state = Auto_Srt_release;
		}
		break;
		
		case Auto_Srt_release:									//Select How much time to wait to Start
			if(button1 && !button2 && ! button3)
			{
				LCD_DisplayString(1,"Inside 1/2 Sec");
				delay_ms(1500);
				state = Auto_P1_press;
			}
			
			 if(button2 && !button1 && ! button3)
			{
				LCD_DisplayString(1,"Inside 1 MIN");
				delay_ms(1500);
				state = Auto_P2_press;
			}
		break;
		
		case Auto_P1_press:
		if(button1 && !button2 && ! button3)
		{
			state = Auto_P1_press;
		}
		
		else if(!button1 && !button2 && ! button3)
		{
			TimeHolder = 0x01;
			state = Auto_P1_rlse;

		}
		
		break;
		
		case Auto_P2_press:
		if(!button1 && button2 && ! button3)
		{
			state = Auto_P2_press;
		}
		
		else if(!button1 && !button2 && ! button3)
		{
			TimeHolder = 0x02;
			state = Auto_P2_rlse;
			
		}
		
		case Auto_P1_rlse:
		Auto_Timer();
		state = Geni_release;
		LCD_DisplayString(1,"Engine Starting...Vroom....Vrooom");
		delay_ms(1500);
		break;
		
		case Auto_P2_rlse:
		Auto_Timer();
		state = Geni_release;
		LCD_DisplayString(1,"Engine Starting...Vroom....Vrooom");
		delay_ms(1500);
		break;
		
		case Sht_Dwn_press:
		if(button3&&!button1&&!button2)
		{
			state = Sht_Dwn_press;
		}
		
		else if(!button3&&!button2&&!button1)
		{
			state = Sht_Dwn_rlse;
			
		}
		
		case Sht_Dwn_rlse:
		state = idle;
		break;
		
		default:
		break;
	}
	
	switch(state)
	{
		case idle:
		LCD_DisplayString(1,"In Idle");
		PORTB = SetBit1(PORTD,3,0);
		break;
		
		case Geni_press:
		LCD_DisplayString(1,"Pressing Button1");
		break;
		
		case  Geni_release:
		LCD_DisplayString(1,"Engine Starting...Vroom....Vrooom");
		delay_ms(1500);
		PORTB = SetBit1(PORTB,3,1);
		PORTB = SetBit1(PORTB,2,1);
		break;
		
		case Sensor_Polling:
		Voltage_Sampler();												//Line 1 DC Voltage OUTPUT READING

		break;
		
		case Auto_Strt_press:
		LCD_DisplayString(1,"Selected Auto Start");
		delay_ms(1000);
		break;
		
		case Auto_Srt_release:
		LCD_DisplayString(1,"How long to wait to Start?");
		delay_ms(2000);
		Auto_Timer();
		break;
		
		case Sht_Dwn_press:
		LCD_DisplayString(1,"Pressed Shutting Down");
		delay_ms(5000);
		break;
		
		case Sht_Dwn_rlse:
		PORTB = SetBit1(PORTB,3,0);
		PORTB = SetBit1(PORTB,2,0);
		LCD_DisplayString(1,"Shutting Down");
		delay_ms(2000);
		break;
		
		case Auto_P1_press:
		LCD_DisplayString(1,"Pressed Button1Auto");
		delay_ms(1500);
		break;
		
		case Auto_P1_rlse:
		LCD_DisplayString(1,"Released Button1Auto");
		break;
		
		case Auto_P2_press:
		LCD_DisplayString(1,"Pressed Button2Auto");
		
		break;
		
		case Auto_P2_rlse:
		LCD_DisplayString(1,"Released Button2Auto");
		break;
		
		case BlueTooth_ON_Prss:
		LCD_DisplayString(1,"Selected Bluetooth Mode: Button4");
		delay_ms(5000);
		break;
		
		case BlueTooth_ON_Rlse:
		LCD_DisplayString(1,"Waiting for Bluetooth Input");
		delay_ms(5000);
		break;
		
		default:
		break;
	}
}
//http://www.geeksforgeeks.org/convert-floating-point-number-string/
// Converts a floating point number to string.
// reverses a string 'str' of length 'len'
void reverse(char *str, int len)
{
	int i=0, j=len-1, temp;
	while (i<j)
	{
		temp = str[i];
		str[i] = str[j];
		str[j] = temp;
		i++; j--;
	}
}
//http://www.geeksforgeeks.org/convert-floating-point-number-string/
// Converts a given integer x to string str[].  d is the number
// of digits required in output. If d is more than the number
// of digits in x, then 0s are added at the beginning.
int intToStr(int x, char str[], int d)
{
	int i = 0;
	while (x)
	{
		str[i++] = (x%10) + '0';
		x = x/10;
	}
	
	// If number of digits required is more, then
	// add 0s at the beginning
	while (i < d)
	str[i++] = '0';
	
	reverse(str, i);
	str[i] = '\0';
	return i;
}
//http://www.geeksforgeeks.org/convert-floating-point-number-string/
// Converts a floating point number to string.
void ftoa(float n, char *res, int afterpoint)
{
	// Extract integer part
	int ipart = (int)n;
	
	// Extract floating part
	float fpart = n - (float)ipart;
	
	// convert integer part to string
	int i = intToStr(ipart, res, 0);
	
	// check for display option after point
	if (afterpoint != 0)
	{
		res[i] = '.';  // add dot
		
		// Get the value of fraction part up to given no.
		// of points after dot. The third parameter is needed
		// to handle cases like 233.007
		fpart = fpart * pow(10, afterpoint);
		
		intToStr((int)fpart, res + i + 1, afterpoint);
	}
}

// http://maxembedded.com/2011/06/the-adc-of-the-avr/
//ADC MUX SELECTOR

void ADC_init()
{
	// AREF = AVcc
	ADMUX = (1<<REFS0);
	
	// ADC Enable and prescaler of 128
	// 16000000/128 = 125000
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

// http://maxembedded.com/2011/06/the-adc-of-the-avr/
//ADC MUX SELECTOR
uint16_t adc_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with '7' will always keep the value
	// of 'ch' between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8)|ch;     // clears the bottom 3 bits before ORing
	
	// start single conversion
	// write '1' to ADSC
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes '0' again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC);
}
uint16_t adc_result0, adc_result1;
float volts;
float temp;
char res[20];

void Voltage_Sampler()
{
	LCD_DisplayString(1,"Voltage Now");
	delay_ms(1000);
	adc_result0 = adc_read(0);      // read adc value at PA0
	x = adc_result0;									//Samples and outputs the voltages;
	volts = x * (5.0/1023.0);
	volts= volts *2;
	temp = volts;
	ftoa((volts),res, 2);
	
	
	LCD_DisplayString(1, res);
	delay_ms(2000);
	Current_Sampler();
	
}


float curent_voltage_in;					//voltage input
float y =0.0;
float pwr = 0.0;
float rpm = 0.0;
float avg = 0.0;
char res2[20];
char res3[20];
void Current_Sampler()
{
	LCD_DisplayString(1,"Current Now");
	delay_ms(1000);
	
	
	
	adc_result1 = adc_read(1);      // read adc value at PA1
	y = adc_result1;
	
	for(int i = 0; i < 30; i++)
	{
		adc_result1 = adc_read(1);      // read adc value at PA1
		y += adc_result1;	
		delay_ms(10);
	}
	avg = y/30;
	
	curent_voltage_in = (530- y) * 27.03 / 1024;;		//10 amp rating
	ftoa(curent_voltage_in,res2,2);
	
	
	LCD_DisplayString(1,res2);
	delay_ms(5000);
	
	
	
}


void Auto_Timer()
{
	LCD_DisplayString(1,"Push Corresponding Button");
	delay_ms(1500);
	LCD_DisplayString(1, " 0.5   1  ...min");
	delay_ms(1500);
	
	if(TimeHolder == 0x01)
	{
		LCD_DisplayString(1,"Starting in -30 sec");
		delay_ms(10000);
		LCD_DisplayString(1,"Starting in -20 sec");
		delay_ms(10000);
		LCD_DisplayString(1,"Starting in -10 sec");
		delay_ms(10000);
		}
		
		if(TimeHolder == 0x02)
		{
			LCD_DisplayString(1,"Starting in -60 sec");
			delay_ms(10000);
			LCD_DisplayString(1,"Starting in -50 sec");
			delay_ms(10000);
			LCD_DisplayString(1,"Starting in -40 sec");
			delay_ms(10000);
			LCD_DisplayString(1,"Starting in -30 sec");
			delay_ms(10000);
			LCD_DisplayString(1,"Starting in -20 sec");
			delay_ms(10000);
			LCD_DisplayString(1,"Starting in -10 sec");
			delay_ms(10000);
		}
		
	
	
	
	
	
}
void ReceiveBluetooth()
{
	if(USART_HasReceived(1))
	{
		
		btemp = USART_Receive(1);
		
		
			if(btemp ==0x01)
			{
				LCD_DisplayString(1,"Received ON:BLuetooth");
				Bluetooth_Status_Slctr = 0x01;
				delay_ms(1500);
			}
			
			if(btemp ==0x00)
			{
				LCD_DisplayString(1,"Received OFF:Bluetooth");
				Bluetooth_Status_Slctr = 0x00;
				delay_ms(1500);
			}
			
			
			
			
			
			
		}
		
	
	
}


int main(void)
{
	state = idle;
	DDRC = 0xFF; PINC = 0x00;		//LCD data lines
	DDRB = 0xFF; PINB = 0x00;		//LCD COntrol lines
	DDRD = 0x00; PIND = 0xFF;		//Master Buttons INPUT
	DDRA =0x00; PORTA = 0xFF;		//input	for ADC
	
	

	 ADC_init();
	   
	initUSART(0);
	initUSART(1);
	TimerOn();
	TimerSet(1000);
	LCD_init();

    while (1) 
    {
		
		Tick();
		
		while(!TimerFlag);
		TimerFlag = 0;	
    }
}

