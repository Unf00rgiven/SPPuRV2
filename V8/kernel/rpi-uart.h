/**
 * @file rpi-uart.h
 * @author Dejan Bogdanovic
 * 
 * RPI's UART.
 *
 * This file contains macros, structures and functions for UART usage.
 */

#ifndef RPI_UART_H
#define RPI_UART_H

#include "rpi-base.h"

/** THe base address of UART peripheral */
#define RPI_UART_BASE			( PERIPHERAL_BASE + 0x201000UL )

typedef struct{
	rpi_reg_rw_t 	DR; 			// Data register
	rpi_reg_rw_t	RSRECR;
	rpi_reg_ro_t    Reserved0;
	rpi_reg_ro_t    Reserved1;
	rpi_reg_ro_t    Reserved2;
	rpi_reg_ro_t    Reserved3;
	rpi_reg_rw_t	FR;				// Flag register
	rpi_reg_ro_t	ILPR;
	rpi_reg_rw_t	IBRD;				// Int part of baud rate divisor
	rpi_reg_rw_t	FBRD;				// Fractional part of baud rate divisor
	rpi_reg_rw_t	LCRH;				// Line control register
	rpi_reg_rw_t	CR;		// Control register
	rpi_reg_rw_t 	IFLS;
	rpi_reg_rw_t	IMSC;
	rpi_reg_rw_t	RIS;
	rpi_reg_rw_t	MIS;
	rpi_reg_rw_t	ICR;
} rpi_uart_t;

/**
 * Get memory mapped area for UART
 *
 * @return starting address of memory area
 */
extern rpi_uart_t * RPI_GetUart(void);

/**
 * Initializing function for UART
 */
extern void RPI_UartInit(void);


#endif	
