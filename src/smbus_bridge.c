/*
 * smbus_bridge.c
 *
 * Created: 6/28/2024 8:16:12 AM
 *  Author: aparady
 */ 

#ifndef F_CPU
#warning "F_CPU not defined!"

#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>

#include "ctype.h"
#include "smbus_bridge.h"
#include "arduino_drivers.h"
#include "arduino_errors.h"

uint8_t broadcast_flag = 0; // If this flag is not zero then the I2C interpreter will not check for an ACK from the slave device when transmitting

uint8_t display_help(){
	uint8_t system_status = NO_ERROR;
	
	uint8_t help_index = 0;
	uint8_t help[] = "I2C Dongle | XX = Hex Addr/Data | XX! = Start + Addr + W | XX? = Start + Addr + R | XX$ = Write Data Byte or ACKs | @ = Find Slave Addresses | ^ = Current I2C Bus State | T = 400kHz | S = 100kHz | V = 10kHz | % = Current Bus Frequency\n";
	
	while(help[help_index] != '\0'){
		system_status |= UART_transmit(help[help_index]);
		help_index++;
		_delay_us(10);
	}
	
	return system_status;
}

uint8_t I2C_scan_addresses(){
	uint8_t I2C_status = NO_ERROR;
	
	// From 0x00 to 0x7F, which is 128 values
	for (int address = 0; address < 129; address++){
		I2C_start();
		if((I2C_status |= I2C_timeout()) != I2C_NO_ERROR) break;
		
		// Check if the current I2C state matches START or repeated START
		if((I2C_get_status() != 0x08) && (I2C_get_status() != 0x10)){
			I2C_status |= I2C_START_FAIL;
			break;
		}
		
		I2C_write(address << 1);
		if((I2C_status |= I2C_timeout()) != I2C_NO_ERROR) break;
		
		// Check if the current I2C state matches SLA+W+ACK or SLA+W+NACK
		if((I2C_get_status() != 0x18) && (I2C_get_status() != 0x20)){
			I2C_status |= I2C_ADDR_NACK;
			break;
		}
		
		// Find devices connected to i2c bus
		if (I2C_get_status() == 0x18){
			system_error_handler(UART_transmit_hex(address));
		}
	}
	
	I2C_stop();
	I2C_release_bus();
	
	return I2C_status;
}

void I2C_broadcast(uint16_t *data_array){
	for(int16_t receive_index = 1; receive_index < data_array[0]; receive_index++){
		
		// If the data byte is greater than 1000, then the data byte refers to the address of the slave
		if (data_array[receive_index] > 1000){
			I2C_start();
			if(I2C_timeout() & I2C_BUS_RESET) break;
			
			I2C_write(data_array[receive_index] - 1000);
			if(I2C_timeout() & I2C_BUS_RESET) break;
		}
		
		else{
			// If SLA+R was transmitted and an ACK was received, then the next data byte should be interpreted as the number of bytes to ACK.
			if (I2C_get_status() == 0x40){
				
				// Exit if desired bytes to read back is zero to avoid a bus error.
				if (data_array[receive_index] == 0){
					break;
				}
				
				// Send ACKs based on data byte value, but send a NACK on the last transmission.
				while(data_array[receive_index] > 1){
					I2C_ACK();
					if(I2C_timeout() & I2C_BUS_RESET) break;
					
					system_error_handler(UART_transmit_hex(I2C_read()));
					
					data_array[receive_index]--;
				}
				I2C_NACK();
				if(I2C_timeout() & I2C_BUS_RESET) break;
				
				system_error_handler(UART_transmit_hex(I2C_read()));
				system_error_handler(UART_transmit('\n'));
			}
			else{
				I2C_write(data_array[receive_index]);
				if(I2C_timeout() & I2C_BUS_RESET) break;
			}
		}
	}
	I2C_stop();
	I2C_release_bus();
}

uint8_t I2C_arbitration(uint16_t *data_array){
	uint8_t I2C_status = I2C_NO_ERROR;
	
	for(int16_t receive_index = 1; receive_index < data_array[0]; receive_index++){
		
		// If the data byte is greater than 1000, then the data byte refers to the address of the slave
		if (data_array[receive_index] > 1000){
			I2C_start();
			if((I2C_status |= I2C_timeout()) & I2C_BUS_RESET) break;
			
			// Check if the current I2C state matches START or repeated START
			if((I2C_get_status() != 0x08) && (I2C_get_status() != 0x10)){
				I2C_status |= I2C_START_FAIL;
				break;
			}
			
			I2C_write(data_array[receive_index] - 1000);
			if((I2C_status |= I2C_timeout()) & I2C_BUS_RESET) break;
			
			// Check if the current I2C state matches SLA+W+ACK or SLA+R+ACK
			if((I2C_get_status() != 0x18) && (I2C_get_status() != 0x40)){
				  I2C_status |= I2C_ADDR_NACK;
				  break;
			}
		}
		
		else{
			// If SLA+R was transmitted and an ACK was received, then the next data byte should be interpreted as the number of bytes to ACK.
			if (I2C_get_status() == 0x40){
				
				// Exit if desired bytes to read back is zero to avoid a bus error.
				if (data_array[receive_index] == 0){
					I2C_status |= I2C_NO_BYTES_REQUESTED;
					break;
				}
				
				// Send ACKs based on data byte value, but send a NACK on the last transmission.
				while(data_array[receive_index] > 1){
					I2C_ACK();
					if((I2C_status |= I2C_timeout()) & I2C_BUS_RESET) break;
					
					// Check if the current I2C state does not match data_byte+ACK
					if(I2C_get_status() != 0x50){
						I2C_status |= I2C_DATA_READ_ACK_FAIL;
						break;
					}
					
					system_error_handler(UART_transmit_hex(I2C_read()));
					
					data_array[receive_index]--;
				}
				I2C_NACK();
				if((I2C_status |= I2C_timeout()) & I2C_BUS_RESET) break;
				
				// Check if the current I2C state does not match data_byte+ACK
				if(I2C_get_status() != 0x58){
					I2C_status |= I2C_DATA_READ_NACK_FAIL;
					break;
				}
				
				system_error_handler(UART_transmit_hex(I2C_read()));
				system_error_handler(UART_transmit('\n'));
			}
			else{
				I2C_write(data_array[receive_index]);
				if((I2C_status |= I2C_timeout()) & I2C_BUS_RESET) break;
			}
		}
	}
	I2C_stop();
	I2C_status |= I2C_release_bus();
	
	return I2C_status;
}

uint8_t UART_receive_array(uint8_t I2C_status){
	// 258 is the maximum bytes that a single address/command can be. The first byte in the array is dedicated to the index, so instead of 259, the buffer must be 260 to allow for overflow checking/handling
	uint16_t UART_receive_buffer_length = 261;
	uint16_t UART_receive_buffer[UART_receive_buffer_length];
	UART_receive_buffer[0] = 1;
	
	uint8_t stacked_data = 0; // Initializer for incoming data to be concatenated. This is an 8 bit value to prevent sending too large of a value over I2C by rolling over when greater than 255
	char UART_data = '\0'; // Initialize to a known state
	uint8_t special_char = 0; // If a special character is detected, then prevent the I2C transaction from taking place
	
	uint8_t message_index = 0;
	uint8_t enabled_message[] = "Broadcast Mode Enabled!\n";
	uint8_t disabled_message[] = "Broadcast Mode Disabled!\n";
	
	while ((UART_data != '\n') && (UART_receive_buffer[0] < UART_receive_buffer_length)){
		while (!(UCSR0A & (1<<RXC0))); // Wait for data to be received, it is okay for this to be an unbounded loop, as we want to keep looking until there is something here.

		UART_data = toupper(UDR0); // Get received data and convert it to uppercase
		
		switch(UART_data){
			case 'A' ... 'F': // Convert received char data to int and fill up the lower nibble in stacked data by shifting up previous lower nibble to upper nibble.
				stacked_data = (stacked_data * 16) + (UART_data - 55);
				break;
			
			case '0' ... '9': // Convert received char data to int and fill up the lower nibble in stacked data by shifting up previous lower nibble to upper nibble.
				stacked_data = (stacked_data * 16) + (UART_data - 48);
				break;
			
			case '!': // Start + ADDR + W indicator
				UART_receive_buffer[UART_receive_buffer[0]] = (stacked_data << 1) + 1000;
				UART_receive_buffer[0]++;
				stacked_data = 0;
				break;
			
			case '?': // Start + ADDR + R indicator
				UART_receive_buffer[UART_receive_buffer[0]] = ((stacked_data << 1) + 1) + 1000;
				UART_receive_buffer[0]++;
				stacked_data = 0;
				break;
			
			case '$': // data byte indicator
				UART_receive_buffer[UART_receive_buffer[0]] = (stacked_data);
				UART_receive_buffer[0]++;
				stacked_data = 0;
				break;
			
			case '&': // Block data byte indicator or PEC read indicator if used with normal data byte
				UART_receive_buffer[UART_receive_buffer[0]] = (stacked_data + 1);
				UART_receive_buffer[0]++;
				stacked_data = 0;
				break;
			
			case '#': // Block data byte PEC indicator
				UART_receive_buffer[UART_receive_buffer[0]] = (stacked_data + 2);
				UART_receive_buffer[0]++;
				stacked_data = 0;
				break;
			
			case '@': // Find all addresses connected to bus
				if (I2C_status != I2C_NO_ERROR) break;
				
				I2C_status = I2C_scan_addresses();
				system_error_handler(UART_transmit('\n'));
				
				special_char = 1;
				break;
			
			case '%': // Returns current transmission speed in kHz
				if (I2C_status != I2C_NO_ERROR) break;
			
				switch(I2C_get_speed()){
					case 0x48:
						system_error_handler(UART_transmit('1'));
						system_error_handler(UART_transmit('0'));
						break;
					case 0x0C:
						system_error_handler(UART_transmit('4'));
						system_error_handler(UART_transmit('0'));
						break;
					case 0xC0:
						system_error_handler(UART_transmit('1'));
						break;
				}
				system_error_handler(UART_transmit('0'));
				system_error_handler(UART_transmit('\n'));
			
				special_char = 1;
				break;
			
			case '^': // Returns current state of I2C bus and attempts to reset the bus if an error is detected
				system_error_handler(UART_transmit_hex(I2C_status));
				system_error_handler(UART_transmit('\n'));
				
				if (I2C_status != I2C_NO_ERROR) I2C_reset_bus();
				
				I2C_status = I2C_NO_ERROR;
				
				special_char = 1;
				break;
				
			case '*':
				message_index = 0;

				if(stacked_data == 0){
					broadcast_flag = 0;
					
					while(disabled_message[message_index] != '\0'){
						system_error_handler(UART_transmit(disabled_message[message_index]));
						message_index++;
					}
				}
				else{
					broadcast_flag = 1;
					
					while(enabled_message[message_index] != '\0'){
						system_error_handler(UART_transmit(enabled_message[message_index]));
						message_index++;
					}
				}

				special_char = 1;
				break;
			
			case 'H': // Display help and hot keys
				system_error_handler(display_help());
				
				special_char = 1;
				break;
			
			case 'I': // Returns Identity of dongle
				system_error_handler(UART_transmit('='));
				system_error_handler(UART_transmit('\n'));
				
				special_char = 1;
				break;

			case 'S': // Set transmission speed to slow (100Khz)
				if (I2C_status != I2C_NO_ERROR) break;
			
				system_error_handler(I2C_set_speed_standard());

				special_char = 1;
				break;
			
			case 'T': // Set transmission speed to fast (400Khz)
				if (I2C_status != I2C_NO_ERROR) break;
				
				system_error_handler(I2C_set_speed_fast());
				
				special_char = 1;
				break;
			
			case 'V': // Set transmission speed to very slow (10Khz)
				if (I2C_status != I2C_NO_ERROR) break;
				
				system_error_handler(I2C_set_speed_very_slow());
			
				special_char = 1;
				break;

			default:
				break;
		}
	}
	// If the buffer has more than 258 elements, then there was an overflow
	if(UART_receive_buffer[0] > (UART_receive_buffer_length - 1)) system_error_handler(UART_RECEIVE_DATA_OVERFLOW);
	
	if((I2C_status == I2C_NO_ERROR) && (special_char == 0)){
		if(broadcast_flag == 0){
			I2C_status = I2C_arbitration(UART_receive_buffer);
		}
		
		else{
			I2C_broadcast(UART_receive_buffer);
		}
	}
	
	return I2C_status;
}
