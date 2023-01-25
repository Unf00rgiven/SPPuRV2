/**
 * @file rpi-systimer.c
 * @author Brian Sidebotham 
 *
 * ARM system timer. 
 *
 * This file contains function for ARM's system timer usage.
 *
 * Copyright (c) 2013-2018, Brian Sidebotham <brian.sidebotham@gmail.com> 
 */

#include <stdint.h>
#include "rpi-systimer.h"

static rpi_sys_timer_t* rpiSystemTimer = (rpi_sys_timer_t*)RPI_SYSTIMER_BASE;

rpi_sys_timer_t* RPI_GetSystemTimer(void)
{
    return rpiSystemTimer;
}

void RPI_WaitMicroSeconds( uint32_t us )
{
    volatile uint32_t ts = rpiSystemTimer->counter_lo;

    while( ( rpiSystemTimer->counter_lo - ts ) < us )
    {
        /* BLANK */
    }
}
