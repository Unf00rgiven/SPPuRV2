/**
 * @file rpi-base.h
 * @author Brian Sidebotham 
 * 
 * RPI base peripheral addresses.
 *
 * Different models of RPI have different base starting address. Standard 
 * register types are defined here.
 *
 * Copyright (c) 2013-2018, Brian Sidebotham <brian.sidebotham@gmail.com> 
 */

#ifndef RPI_BASE_H
#define RPI_BASE_H

#include <stdint.h>

#if defined( RPI0 ) || defined( RPI1 )
    #define PERIPHERAL_BASE       0x20000000UL
#elif defined( RPI2 ) || defined( RPI3 )
    #define PERIPHERAL_BASE       0x3F000000UL
#elif defined( RPI4 )
    #define PERIPHERAL_BASE       0xFE000000UL
#else
    #error Unknown RPI Model!
#endif

typedef volatile uint32_t rpi_reg_rw_t;
typedef volatile const uint32_t rpi_reg_ro_t;
typedef volatile uint32_t rpi_reg_wo_t;

typedef volatile uint64_t rpi_wreg_rw_t;
typedef volatile const uint64_t rpi_wreg_ro_t;

#endif // RPI_BASE_H
