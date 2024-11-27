/*
 * errors_328P.h
 *
 * Created: 6/28/2024 7:30:17 AM
 *  Author: aparady
 */ 


#ifndef ERRORS_328P_H_
#define ERRORS_328P_H_

/*
All of these error codes require human intervention if they occur since a power cycle is the only effective method of dealing with them.
A watchdog timer will not work since it will kill the serial terminal on the PC side (Arduino devices have the reset pin of the USB CDC device connected to the reset pin of the ATMEGA328P).
*/

enum SYSTEM_ERROR_CODES{
	NO_ERROR,						// No problems reported
	UART_ENABLE_FAIL,				// UART peripheral failed to enable properly and remained disabled
	UART_TRANSMISSION_TIMEOUT,		// Timeout expired when checking to see if the UART peripheral finished transmitting a byte
	I2C_ENABLE_FAIL,				// I2C peripheral failed to enable properly and remained disabled
	I2C_DISABLE_FAIL,				// I2C peripheral failed to disable properly and remained enabled
	I2C_BAUD_FAIL,					// I2C peripheral failed to change baud rate register when commanded
	INCORRECT_GPIO_CONFIGURATION,	// GPIO output register failed to be set to the expected value
	UART_RECEIVE_DATA_OVERFLOW		// The data received over UART is too big to fit into memory
};

/*
Any of these errors will terminate the current I2C transaction and force SCL to pulse to shake off the slave.
The current error can be read back over UART and the on-board led will be lit solid to indicate that an I2C fault occurred.
*/

enum I2C_ERROR_CODES{
	I2C_NO_ERROR						= 0x00,	// No problems reported
	I2C_START_FAIL						= 0x01,	// Failed to issue START or repeated START condition
	I2C_ADDR_NACK						= 0x02,	// Slave address + W/R was NACK'd
	I2C_MASTER_WRITE_ARBITRATION_LOST	= 0x04,	// Master is no longer controlling the bus
	I2C_DATA_READ_ACK_FAIL				= 0x08,	// A read data byte should have been ACK'd but was not
	I2C_DATA_READ_NACK_FAIL				= 0x10,	// A read data byte should have been NACK'd but was not
	I2C_BUS_RESET						= 0x20,	// A timeout expired and the master needed to pulse SCL to reset the bus
	I2C_NO_BYTES_REQUESTED				= 0x40,	// Zero bytes were requested from slave device during a read
	I2C_RESERVED						= 0x80	// Reserved for future use
};

void system_error_handler(uint8_t state);

#endif /* ERRORS_328P_H_ */
