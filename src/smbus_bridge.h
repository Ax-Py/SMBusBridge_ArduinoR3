/*
 * smbus_bridge.h
 *
 * Created: 6/28/2024 7:29:25 AM
 *  Author: aparady
 */ 

#ifndef SMBUS_BRIDGE_H_
#define SMBUS_BRIDGE_H_

uint8_t display_help();

uint8_t I2C_scan_addresses();												// Find all devices connected to I2C bus

void I2C_broadcast(uint16_t *data_array);						// I2C interpreter that does not check if any slave devices are connected. This can be used to check that the master is operating properly on its own.

uint8_t I2C_arbitration(uint16_t *data_array);			// I2C interpreter that checks the state machine of the I2C peripheral. This can only be used when there is at least one slave device on the bus.

uint8_t UART_receive_array(uint8_t data_byte);		  // Receive data from PC serial terminal and parse it according to its value

#endif /* SMBUS_BRIDGE_H_ */
