/*
 * SMBus_Bridge.c
 *
 * Created: 6/28/2024 7:23:56 AM
 * Author : aparady
 */ 

#define F_CPU 16000000UL

#include <avr/io.h>

extern "C" {
#include "src/bridge_commands_328P.h"
#include "src/peripheral_drivers_328P.h"
#include "src/errors_328P.h"
}

uint8_t I2C_status = I2C_NO_ERROR;

int main(void)
{
  DDRB |= 0b001111111;    // Make PB0-5 outputs for error indicators
  PORTB &= ~(0b00111111);   // Make PB0-5 LOW by default
  
  DDRD |= 0b11100000;     // Make PD7/6/5 an output
  PORTD |= 0b01000000;    // Make PD6 HIGH by default for negative pulses
  
  DDRC |= 0b00110000;     // Make only the I2C pins (PC4/5) outputs
  
  UART_init(1000000, 1);
  I2C_init();
  
    /* Replace with your application code */
    while (1) 
    {
    if (I2C_status != I2C_NO_ERROR){
       PORTB |= (1 << PORTB5);
    }else{
      PORTB &= ~(1 << PORTB5);
    }
    
    I2C_status = UART_receive_array(I2C_status);
    }
}
