/*
 * errors_328P.c
 *
 * Created: 6/28/2024 8:18:54 AM
 *  Author: aparady
 */ 

#ifndef F_CPU
#warning "F_CPU not defined!"

#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "errors_328P.h"

// Force an infinite loop where the on-board LED blinks in accordance to the current error code. The rest of PORTB is used to provide a binary representation of the error code.
void system_error_handler(uint8_t state){
	
	if(state == NO_ERROR) return;
	
	PORTB &= ~(1 << PORTB5);
	
	PORTB |= state;
	
	while(1){
		for(uint8_t blinks = 0; blinks < state; blinks++){
			PORTB ^= (1 << PORTB5);
			_delay_ms(500);
			PORTB ^= (1 << PORTB5);
			_delay_ms(500);
		}
		_delay_ms(1000);
	}
}