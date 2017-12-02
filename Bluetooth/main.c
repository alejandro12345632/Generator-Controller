/*
 * Slave.c
 *
 * Created: 10/4/2017 2:44:26 PM
 * Author : Alejandro Garcia
 */ 

#include <avr/io.h>
#include "usart_ATmega1284.h"
#include <avr/interrupt.h>
volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
float x;
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

// Returns a non-zero number if the desired USART has received a byte of data.
// Returns 0 if the desired USART has NOT received a byte of data.
unsigned char USART_HasReceived(unsigned char usartNum);

// Returns the data received on RXD pin of the desired USART.
// Call this function after USART_HasReceived returns 1.
unsigned char USART_Receive(unsigned char usartNum);



int main(void)
{
	//unsigned char temp =0x00;	    // variable used to store received data
	DDRA = 0xFF; PINA = 0x00;
	char temp;
	
	initUSART(0);
	initUSART(1);
	TimerOn();
	TimerSet(8000);
    /* Replace with your application code */
    while (1) 
    {
		
		PORTA = 0x01;
		while(!TimerFlag);
		TimerFlag =0;
		
		PORTA = 0x00;
		while(!TimerFlag);
		TimerFlag =0;
		
			
			if(USART_HasReceived(0))
			{
				PORTB = 0x01;
				temp = USART_Receive(0);
				
				if(temp ==0x01)
				{
					PORTA = 0x01;
					USART_Send('1',0);
					
					USART_Send(temp,1);
					while(!TimerFlag);
					TimerFlag = 0;
						
					
					
				}
				
				 if(temp == 0x00)
				{
					PORTA = 0x00;
					USART_Send('0',0);
					USART_Send(temp,1);
				}
				
				
				
				
			}
			
    }
}

