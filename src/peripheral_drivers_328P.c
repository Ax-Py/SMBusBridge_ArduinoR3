/*
 * peripheral_drivers_328P.c
 *
 * Created: 6/28/2024 7:30:52 AM
 *  Author: aparady
 */ 

#ifndef F_CPU
#warning "F_CPU not defined!"

#define F_CPU 16000000UL
#endif

#define peripheral_timeout F_CPU >> 7

#include <avr/io.h>
#include <compat/twi.h>
#include <util/delay.h>

#include "peripheral_drivers_328P.h"
#include "errors_328P.h"

/*

UART peripheral specific low level commands

*/

uint8_t UART_init(unsigned long baud, uint8_t double_speed){
	// Set baud rate using high and low bit
	UBRR0H = (((F_CPU / (baud * (F_CPU/1000000))) - 1) >> 8);
	UBRR0L = (((F_CPU / (baud * (F_CPU/1000000))) - 1));
	
	// Enable or disable double speed transfers (multiply baud rate by 2)
	UCSR0A |= (double_speed << U2X0);

	// Enable receive and transmit, then check if they were enabled.
	UCSR0B |= (1 << RXEN0)|(1 << TXEN0);
	
	return ((!(UCSR0B & (1 << RXEN0))) || (!(UCSR0B & (1 << TXEN0)))) ? UART_ENABLE_FAIL : NO_ERROR;
}

uint8_t UART_transmit(uint8_t data){
	uint32_t timeout_counter = 0;
	
	// Wait for transmit buffer to be empty. Then put the new data character into the FIFO buffer.
	while (!(UCSR0A & (1 << UDRE0)));
	
	UDR0 = data;
	
	// Wait for the transmission to complete
	while (!(UCSR0A & (1 << TXC0))){
		if(timeout_counter >= peripheral_timeout) break;
		timeout_counter++;
	}
	
	return (timeout_counter >= peripheral_timeout) ? UART_TRANSMISSION_TIMEOUT : NO_ERROR;
}

uint8_t UART_transmit_hex(uint8_t data){
	uint8_t system_status = NO_ERROR;
	
	char ASCII_Table[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'}; // Lookup table for conversion from hex to ASCII char
	
	system_status |= UART_transmit(ASCII_Table[data >> 4]);
	system_status |= UART_transmit(ASCII_Table[data & 0x0F]);
	system_status |= UART_transmit('$');
	
	return system_status;
}

/*

I2C/SMBus/PMBus peripheral specific low level commands

*/

uint8_t I2C_init(){
	// Setup I2C to start with 100kHz transaction frequency
	TWSR = ((0 << TWPS0) | (0 << TWPS1));
	TWBR = (((F_CPU/100000)/1)-16)/2;
	
	// Enable TWI
	TWCR |= (1 << TWEN);
	
	return (!(TWCR & (1 << TWEN))) ? I2C_ENABLE_FAIL : NO_ERROR;
}

void I2C_start(){
	TWCR = ((1<<TWINT) | (1<<TWEN) | (1<<TWSTA));
}

void I2C_write(uint8_t data){
	TWDR = data;
	TWCR = ((1<< TWINT) | (1<<TWEN));
}

void I2C_ACK(){
	TWCR = ((1 << TWINT) | (1 << TWEN) | (1 << TWEA));
}

void I2C_NACK(){
	TWCR = ((1<< TWINT) | (1<<TWEN));
}

uint8_t I2C_read(){
	return TWDR;
}

void I2C_stop(){
	TWCR = ((1<<TWINT) | (1<<TWEN) | (1<<TWSTO));
}

uint8_t I2C_get_status(){
	return TWSR & 0xF8;
}

void I2C_reset_bus(){
	// Turn off I2C peripheral and check that it disabled
	TWCR &= ~(1 << TWEN);
	
	if (TWCR & (1 << TWEN)){
		system_error_handler(I2C_DISABLE_FAIL);
	}
	
	// Set SDA and SCL as LOW outputs and pulse SCL 10 times @ 100kHz to attempt to release slave devices
	DDRC |= ((1 << PORTC5) | (1 << PORTC4));
	PORTC &= ~((1 << PORTC5) | (1 << PORTC4));

	for(int pulses = 0; pulses < 20; pulses++){
		PORTC ^= (1 << PORTC5);
		_delay_us(10);
	}
	
	// Turn back on I2C peripheral and then check that it enabled
	TWCR |= (1 << TWEN);
	
	if (!(TWCR & (1 << TWEN))){
		system_error_handler(I2C_ENABLE_FAIL);;
	}
}

uint8_t I2C_timeout(){
	uint32_t timeout_counter = 0;
	uint8_t I2C_status = I2C_NO_ERROR;
	
	// Check if TWINT was set to indicate that the last command was completed
	while(!(TWCR & (1 << TWINT))){
		
		// Check if the bus release timeout has elapsed
		if(timeout_counter >= peripheral_timeout){
			I2C_reset_bus();
			
			I2C_status = I2C_BUS_RESET;
			break;
		}
		else{
			timeout_counter++;
		}
	}
	return I2C_status;
}

uint8_t I2C_release_bus(){
	uint32_t timeout_counter = 0;
	uint8_t I2C_status = I2C_NO_ERROR;
	
	// A failed command or hung bus will prevent the stop bit from being set
	while(!(TWCR & (1 << TWSTO))){
		
		// Check if the bus release timeout has elapsed
		if(timeout_counter >= peripheral_timeout){
			I2C_reset_bus();
			
			I2C_status = I2C_BUS_RESET;
			break;
		}
		else{
			timeout_counter++;
		}
	}
	return I2C_status;
}

uint8_t I2C_get_speed(){
	return TWBR;
}

uint8_t I2C_set_speed_very_slow(){
	uint8_t frequency = (((F_CPU/10000)/4)-16)/2;
	
	TWSR = ((1<<TWPS0) | (0<<TWPS1));
	TWBR = frequency;
	
	return (I2C_get_speed() != frequency) ? I2C_BAUD_FAIL : NO_ERROR;
}

uint8_t I2C_set_speed_standard(){
	uint8_t frequency = (((F_CPU/100000)/1)-16)/2;
	
	TWSR = ((0<<TWPS0) | (0<<TWPS1));
	TWBR = frequency;
	
	return (I2C_get_speed() != frequency) ? I2C_BAUD_FAIL : NO_ERROR;
}

uint8_t I2C_set_speed_fast(){
	uint8_t frequency = (((F_CPU/400000)/1)-16)/2;
	
	TWSR = ((0<<TWPS0) | (0<<TWPS1));
	TWBR = frequency;
	
	return (I2C_get_speed() != frequency) ? I2C_BAUD_FAIL : NO_ERROR;
}