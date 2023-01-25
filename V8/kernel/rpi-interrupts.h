/**
 * @file rpi-interrupts.h
 * @author Brian Sidebotham 
 * 
 * ARM interrupts.
 *
 * This file contains macros, structures and functions that are needed
 * for handling ARM interrupts.
 *
 * Copyright (c) 2013-2018, Brian Sidebotham <brian.sidebotham@gmail.com> 
 * Modified by Dejan Bogdanovic
 */

#ifndef RPI_INTERRUPTS_H
#define RPI_INTERRUPTS_H

#include <stdint.h>

#include "rpi-base.h"
#include "types-mt.h"

/** @brief See Section 7.5 of the BCM2836 ARM Peripherals documentation, the base
    address of the controller is actually xxxxB000, but there is a 0x200 offset
    to the first addressable register for the interrupt controller, so offset the
    base to the first register */
#define RPI_INTERRUPT_CONTROLLER_BASE   ( PERIPHERAL_BASE + 0xB200 )

/* Bits in the Enable_Basic_IRQs register to enable various interrupts.
    See the BCM2835 ARM Peripherals manual, section 7.5 */
#define RPI_BASIC_ARM_TIMER_IRQ         (1 << 0)
#define RPI_BASIC_ARM_MAILBOX_IRQ       (1 << 1)
#define RPI_BASIC_ARM_DOORBELL_0_IRQ    (1 << 2)
#define RPI_BASIC_ARM_DOORBELL_1_IRQ    (1 << 3)
#define RPI_BASIC_GPU_0_HALTED_IRQ      (1 << 4)
#define RPI_BASIC_GPU_1_HALTED_IRQ      (1 << 5)
#define RPI_BASIC_ACCESS_ERROR_1_IRQ    (1 << 6)
#define RPI_BASIC_ACCESS_ERROR_0_IRQ    (1 << 7)

#define ARM_I_BIT 0x80    /**< IRQs disabled when set to 1. */
#define ARM_F_BIT 0x40    /**< FIQs disabled when set to 1. */


#define INT15 0x15 /**< Not used*/
#define INT60 0x60 /**< Softvare interrupt code. */
#define INT70 0x70 /**< Timer interrupt code. */

/** @brief The interrupt controller memory mapped register set */
typedef struct {
    volatile uint32_t IRQ_basic_pending;
    volatile uint32_t IRQ_pending_1;
    volatile uint32_t IRQ_pending_2;
    volatile uint32_t FIQ_control;
    volatile uint32_t Enable_IRQs_1;
    volatile uint32_t Enable_IRQs_2;
    volatile uint32_t Enable_Basic_IRQs;
    volatile uint32_t Disable_IRQs_1;
    volatile uint32_t Disable_IRQs_2;
    volatile uint32_t Disable_Basic_IRQs;
    } rpi_irq_controller_t;

typedef void (*INTFPTR)(struct STACK_FRAME * regs);

/**
 * Get memory mapped area for interrupt controller
 *
 * @return starting address of memmry area
 */
extern rpi_irq_controller_t* RPI_GetIrqController( void );

/**
 * Set an interrupt handler.
 * 
 * This function sets certain handler in vector table that is specified by vect_num:
 * INT15 - not used.
 * INT60 - will be called every time svc #0x60 instruction
 * is executed.
 * INT70 - will be called every time timer interrupt occurs.
 *
 * @param vect_num code in vector table
 * @param fun function - handler
 */
extern void setvect(uint32_t vect_num, INTFPTR fun);

/**
 * Get an interrupt handler.
 * 
 * This function returns certain handler from vector table that is specified by vect_num.
 *
 * @param vect_num code in vector table
 * @return function - handler
 */
extern INTFPTR getvect(uint32_t vect_num);

/**
 * Timer interrupt handler - lower level.
 *
 * This is the timer interrupt handler that is called directly from assembly.
 * It gets current task's context and calls higher handler from vector table.
 * Timer interrupt bit is cleared and timer counter is reset.
 *
 * @param regs pointer to current task's context
 */
extern void irq_handler(struct STACK_FRAME * regs);
/**
 * Supervisor call handler - lower level.
 *
 * This is the supervisor call handler that is called directly from assembly.
 * It gets current task's context and calls higher handler from vector table.
 *
 * @param regs pointer to current task's context
 */
extern void svc_handler(struct STACK_FRAME * regs);

#define enable() asm("cpsie	i")      /**< Macro for enabling interrupts. */
#define disable() asm("cpsid	i")  /**< Macro for disabling interrupts. */

#endif // RPI_INTERRUPTS_H
