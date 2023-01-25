/**
 * @file rpi-uart.h
 * @author Dejan Bogdanovic
 * 
 * RPI's UART.
 *
 * This file contains functions for UART usage.
 */

#include <stdint.h>
#include "rpi-uart.h"

static rpi_uart_t * rpiUart = (rpi_uart_t *)RPI_UART_BASE;

rpi_uart_t * RPI_GetUart(void){
	return rpiUart;
}

void RPI_UartInit(void){

}
