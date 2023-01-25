/**
 * @file rpi-systimer.h
 * @author Brian Sidebotham 
 *
 * ARM system timer. 
 *
 * This file contains macros, structures and function for ARM's system timer
 * usage.
 *
 * Copyright (c) 2013-2018, Brian Sidebotham <brian.sidebotham@gmail.com> 
 */
#ifndef RPI_SYSTIMER_H
#define RPI_SYSTIMER_H

#include <stdint.h>
#include "rpi-base.h"

#define RPI_SYSTIMER_BASE       ( PERIPHERAL_BASE + 0x3000 )


typedef struct {
    volatile uint32_t control_status;
    volatile uint32_t counter_lo;
    volatile uint32_t counter_hi;
    volatile uint32_t compare0;
    volatile uint32_t compare1;
    volatile uint32_t compare2;
    volatile uint32_t compare3;
    } rpi_sys_timer_t;

/**
 * Get memory mapped area for System timer
 *
 * @return starting address of memory area
 */
extern rpi_sys_timer_t* RPI_GetSystemTimer(void);

/**
 * Busy waiting
 *
 * This function reads counter value at the begining and waits until 
 * enough ticks have passed.
 *
 * @param us amount of us to wait
 */
extern void RPI_WaitMicroSeconds( uint32_t us );

#endif // RPI_SYSTIMER_H

