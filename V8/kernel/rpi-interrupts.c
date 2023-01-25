/**
 * @file rpi-interrupts.c
 * @author Brian Sidebotham 
 * 
 * ARM interrupts.
 *
 * This file contains functions that are needed for handling ARM interrupts.
 *
 * Copyright (c) 2013-2018, Brian Sidebotham <brian.sidebotham@gmail.com> 
 * Modified by Dejan Bogdanovic
 */

#include <stdint.h>

#include "rpi-interrupts.h"
#include "rpi-armtimer.h"
#include "types-mt.h"
#include "rpi-aux.h"

/** @brief The BCM2835 Interupt controller peripheral at it's base address */
static rpi_irq_controller_t* rpiIRQController =
        (rpi_irq_controller_t*)RPI_INTERRUPT_CONTROLLER_BASE;

/**
 * @brief Return the IRQ Controller register set
 */
rpi_irq_controller_t* RPI_GetIrqController( void )
{
    return rpiIRQController;
}

/** 
 * Default interrupt handler.
 *
 * Initially interrupt handlers are empty functions.
 */
static void default_handler(struct STACK_FRAME * regs){}

/** 
 * Vector table.
 *
 * Three interrupt handlers - INT15, INT60, INT70.
 */
static INTFPTR vectors[3] = {default_handler, default_handler, default_handler};

void setvect(uint32_t vect_num, INTFPTR fun){
	switch(vect_num){
		case INT15:
			vectors[0] = fun;
		break;
		case INT60:
			vectors[1] = fun;
		break;
		case INT70:
			vectors[2] = fun;
		break;
	}
}

INTFPTR getvect(uint32_t vect_num){
	switch(vect_num){
		case INT15:
			return vectors[0];
		case INT60:
			return vectors[1];
		case INT70:
			return vectors[2];
	}
}
 
void irq_handler(struct STACK_FRAME * regs){
	RPI_GetArmTimer()->IRQClear = 1;
	vectors[2](regs);
	RPI_GetArmTimer()->Load = 0x58; // around 0.025ms
}

void svc_handler(struct STACK_FRAME * regs){
	vectors[1](regs);
}
