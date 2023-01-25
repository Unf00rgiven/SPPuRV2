/**
 * @file arm.h 
 * @author Dejan Bogdanovic
 *
 * ARM processor modes.
 *
 * This file contains macros for 8 different processor modes and
 * one mask. Mode change is done by executing  cps #mode instruction.
 */


#ifndef ARM_H
#define ARM_H

/**
 * Arm processor mode mask.
 * 
 * In order to get the processor mode, read the CPSR and
 * apply this mask since last five bits contain processor mode.
 */
#define ARM_MODE_MASK  0x1f

#define ARM_MODE_USR   0x10    /**< User mode */
#define ARM_MODE_FIQ   0x11    /**< Fast interrupts mode */
#define ARM_MODE_IRQ   0x12    /**< Standard interrupts mode */
#define ARM_MODE_SVC   0x13    /**< Software interrupts (supervisor) mode */
#define ARM_MODE_ABT   0x17    /**< Abort mode */
#define ARM_MODE_HYP   0x1A    /**< Hyper user mode */
#define ARM_MODE_UND   0x1B    /**< Undefined instructions mode */
#define ARM_MODE_SYS   0x1F    /**< System mode */

#endif // ARM_H
