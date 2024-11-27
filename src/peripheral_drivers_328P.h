/*
 * peripheral_drivers_328P.h
 *
 * Created: 6/28/2024 7:24:46 AM
 *  Author: aparady
 */ 


#ifndef PERIPHERAL_DRIVERS_328P_H_
#define PERIPHERAL_DRIVERS_328P_H_

#include <avr/io.h>

/*

UART specific low level commands

*/

uint8_t UART_init(unsigned long baud, uint8_t double_speed);

uint8_t UART_transmit(uint8_t data);

uint8_t UART_transmit_hex(uint8_t data);

/*

I2C/SMBus specific low level commands

*/

uint8_t I2C_init();

void I2C_start();

void I2C_write(uint8_t data);

void I2C_ACK();

void I2C_NACK();

uint8_t I2C_read();

void I2C_stop();

uint8_t I2C_get_status();

void I2C_reset_bus();

uint8_t I2C_timeout();

uint8_t I2C_release_bus();

uint8_t I2C_get_speed();

uint8_t I2C_set_speed_very_slow(); // 10kHz

uint8_t I2C_set_speed_standard(); // 100kHz

uint8_t I2C_set_speed_fast(); // 400kHz

#endif /* PERIPHERAL_DRIVERS_328P_H_ */