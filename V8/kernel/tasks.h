/**
 * Copyright Odsek RT-RK - free to use for educational purposes.
 */

#ifndef TASKS_H
#define TASKS_H

/* 
 * Task states
 */
#define TASK_NOT_CREATED    0x00
#define TASK_BACKGND        0x01
#define TASK_BACKGND        0x01
#define TASK_READY          0x02
#define TASK_WAIT_SUSP      0x03
#define TASK_SUSPENDED      0x04
#define TASK_TERMINATED     0x05

/*
 * Suspension reasons
 */
#define SUSPEND_ON_SEM      0
#define SUSPEND_ON_TIME     1
#define PERM_SUSPEND        2
#define TEMP_SUSPEND        3
#define SUSPEND_ON_SHMEMRD  4
#define SUSPEND_ON_SHMEMWR  5

/*
 * Scheduling modes
 */
#define TIME_SLICE          0x01
#define ROUND_ROBIN         0x02
#define TIME_AND_ROUND      TIME_SLICE|ROUND_ROBIN


/*
 * MT timers
 */
#define MTTIMER_INACTIVE      0x00
#define MTTIMER_ACTIVE        0x01

#define MTTIMER_CREATE_ONLY   0x00
#define MTTIMER_START_NOW     0x10
#define MTTIMER_ONE_SHOT      0x00
#define MTTIMER_CYCLIC        0x01

/*
 * Mail box
 */
#define MBOX_DELETED 0
#define MBOX_EMPTY   1
#define MBOX_FULL    2
#define MBOX_MAILS   3
#define MBOX_INUSE   4

#define MAIL_ON_TAIL 0
#define MAIL_ON_HEAD 1

/*
 * Shared memory
 */
#define RW_FREE  0x01
#define RW_READ  0x02
#define RW_WRITE 0x04
#define RW_FIFO  0x08
#define RW_FRREQ 0x10

/*
 * Task properties
 */
#define LRNMIN      1
#define LRNMAX		255

#define PRMIN	    1
#define PRMAX     1000
#define PRDEFAULT 100

// Context len is 17 regs = 17*4
#define CONTEXT_LEN 17*4 
#define STACK_SIZE  10000
#define NO_TIME     0xFFFFFFFF

#endif	//TASKS_H
