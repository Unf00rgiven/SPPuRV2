/**
 * Copyright Odsek RT-RK - free to use for educational purposes.
 */


#ifndef TYPES_MT_H
#define TYPES_MT_H

#include <stdint.h>

/*
 * Define standard types
 */
#ifndef TYPES_DEFINED
#define TYPES_DEFINED
typedef uint8_t 	BYTE;
typedef int8_t  	CHAR;
typedef uint16_t 	WORD;
typedef uint32_t  DWORD;
typedef BYTE * 		BYTE_ptr;
typedef WORD * 		WORD_ptr;
#endif	//TYPES_DEFINED

/**
 * Stack frame (processor context) for ARM.
 *
 * Context is stored on stack and it looks like this:
 *
 * higher adresses:
 * SPSR (CPSR form task)
 * LR (PC - return address)
 * IP
 * FP
 * R10
 * R9
 * R8
 * R7
 * R6
 * R5
 * R4
 * R3
 * R2
 * R1
 * R0
 * LR (task's LR)
 * SP
 * :lower adresses
 */
struct STACK_FRAME{
	uint32_t sp,lr;				/**< Stack pointer, Link register */
	uint32_t r0,r1,r2,r3;		/**< r0-r3 */
	uint32_t r4,r5,r6,r7;		/**< r4-r7 */
	uint32_t r8,r9,r10;			/**< r8-r10 */
	uint32_t fp,ip;				/**< Frame pointer, ip */
	uint32_t pc,cpsr;				/**< Program counter, Current program status register */
};

struct	semafor;

typedef struct mbox {
   int state;
   int head;
   int tail;
   int count;
   int slots;
   int msglen;
   char** mailboxp;
   void *smPutMsg; /* pokazivac na put semafor */
   void *smGetMsg; /* pokazivac na get semafor */
} MBOX;
typedef MBOX* MBP;


// Task descriptor.
struct linked_list {
   struct STACK_FRAME I_REGISTRI;	// Registers
   WORD              	I_LRN;			// Handler.
   WORD              	I_PARENT;	  // Parent handler.
   WORD              	I_STANJE;		// State
   WORD              	I_UZROK;		// Suspension cause.
   unsigned         	I_VREME;
   unsigned          	I_SWITCH; 	// Scheduling method.
   WORD              	I_PRIORITY; // Priority
   void              	(*start_point)(void*); // Starting address
   void*            	params;	// Params
   void*             	stack_mem;	// Stack pointer.
   unsigned          	stack_len;	// Stack length.
   struct            	linked_list *next_ready; // Next ready.
   struct            	linked_list *next_tim_susp; // Next time suspended.
   struct            	linked_list *next_sem_susp; // Next semaphore suspended.
   MBOX              	mails; // Mailbox
   unsigned long     	start_time; // 
   unsigned long     	total_time_low; // 
   unsigned long     	total_time_hi; // 
	} ;

typedef struct linked_list ISA; // Task descriptor 
typedef ISA * LINK;	// Descriptor pointer.

// Semaphore.
struct	semafor	{
	int    S_COUNT;
	struct linked_list* S_QH;
	struct linked_list* S_QT;
	};

// Mutex.
struct	mutex	{
	int    S_COUNT;
	int    S_OWNCNT;
	struct linked_list* S_OWNER;
	struct linked_list* S_QH;
	struct linked_list* S_QT;
	};

typedef	struct	semafor SEM;
typedef	struct	semafor CRIT_SECT; // Crit section.
typedef	SEM	*SEMPTR;	
typedef	struct	mutex MUTEX;
typedef	MUTEX* MTXPTR;
typedef int TASK_HANDLER; // Handler id


// Timer
typedef struct mt_timer
{
   unsigned           cyclus;
   unsigned           deltaT;
   int                timer_triger;
   SEM                timer_sem;
   int                timer_type;
   int                timer_status;
   int                timer_fun_handler;
   int                timer_fun_priority;
   struct mt_timer*   next_timer;
} MTTIMER;
typedef MTTIMER* MTTIMERHND;

// RW Sync
typedef struct RWLockObj
{
	struct linked_list* S_QH;
	struct linked_list* S_QT;
	unsigned char status;
	unsigned int readin_counter;
} RWSEM;

// Binary semaphore
typedef struct binsem
{
	int    S_COUNT;
	int    S_OWNCNT;
	int    ceiling_priority;
	int    org_priority;
	struct linked_list* S_OWNER;
	struct linked_list* S_QH;
	struct linked_list* S_QT;
} BINSEM;

#endif	// TYPES_MT_H
