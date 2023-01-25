/**
 * @file cstartup.c
 * @author Brian Sidebotham 
 *
 * C startup code.
 *
 * This file contains the first function that will be called from assembly.
 * Function has to initialize BSS section and starts the kernel.
 *
 * Copyright (c) 2013-2018, Brian Sidebotham <brian.sidebotham@gmail.com> 
 */

#include "kernel.h"

extern int __bss_start__;  /**< Start of BSS section */
extern int __bss_end__;    /**< End of BSS section */

/**
 * Infinite loop function.
 *
 * Every time you dont expect your code to get to a certain point, you
 * should call halt, because there is no OS to handle these situations.
 */
extern void _halt(void);

/**
 * Function that initializes BSS section and starts kernel.
 * 
 * If for some reason execution returns from kernel_main, call halt.
 * None of the registers that are passed are used.
 *
 * @param r0 register r0 passed from assembly
 * @param r1 register r1 passed from assembly
 * @param r2 register r2 passed from assembly
 */
void _cstartup(unsigned int r0, unsigned int r1, unsigned int r2){
  int* bss = &__bss_start__;
  int* bss_end = &__bss_end__;

  // Clear the bss section.
  while(bss < bss_end){
    *bss++ = 0;
  }

  // Start executing kernel.
  kernel_main(r0, r1, r2);

  // Halt in case we reach this line.    
  _halt();
}
