/**
 * @file rpi-armtimer.c
 * @author Brian Sidebotham 
 * 
 * ARM timer.
 *
 * Detailed description is in rpi-armtimer.h
 *
 * Copyright (c) 2013-2018, Brian Sidebotham <brian.sidebotham@gmail.com> 
 */

#include "rpi-armtimer.h"

#include <stdint.h>

static rpi_arm_timer_t* rpiArmTimer = (rpi_arm_timer_t*)RPI_ARMTIMER_BASE;

rpi_arm_timer_t* RPI_GetArmTimer(void){
    return rpiArmTimer;
}

void RPI_ArmTimerInit(void){
    // Empty.
}
