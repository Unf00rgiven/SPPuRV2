/**
 * Copyright Odsek RT-RK - free to use for educational purposes.
 *
 * RPI multitasking.
 *
 * This file contains all the multitasking functions and test functions.
 */

#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include "tasks.h"
#include "types-mt.h"

#define TRUE   1  
#define FALSE  0

/**
 * Supervisor call (SVC) handler.
 *
 * This function will be called every time SVC instruction is executed.
 * Address of STACK_FRAME structure is passed as argument. All registers from current process are stored in
 * that structure, and if task has to be switched, they are saved in memory and registers of next process
 * are loaded. When this function finishes, registers from this structure are loaded back in CPU's registers.
 * 
 * @param stack pointer to structure holding current task's registers
 */
extern void TaskDisp (struct STACK_FRAME * stack);

/**
 * Timer interrupt handler.
 * 
 * This function will be called every time a timer interrupt occurs. In addition to switching tasks as
 * TaskDisp, this function will also keep track of how long tasks are running.
* 
 * @param stack pointer to structure holding current task's registers
 */
extern void TimeInt (struct STACK_FRAME * stack);

/**
 * Function that disables interrupts.
 *
 * Disables interrupts and returns previous value of CPSR.
 * @return previous value of CPSR
 */
extern int set_disable(void);

/**
 * Function that restores interrupts.
 *
 * Enables interrupts if they were previously enabled in CPSR.
 * @param flags previous value of CPSR
 */
extern void set_enable(int flags);

/**
 * Function that adds a task to ready tasks.
 *
 * Link a task in systems ready queue. Tasks are sorted by priority.
 * @param novi pointer to task descriptor that will be added
 */
extern void queue(LINK novi);

/**
 * Function that adds a task in semaphores queue.
 *
 * Link a task in semaphore queue. Tasks are sorted by priority.
 *
 * @param S_QH pointer to task descriptor that is first in the list
 * @param S_QT pointer to task descriptor that is last in the list
 * @param novi task that will be added
 */
extern void semqueue_by_priority ( LINK *S_QH, LINK* S_QT, LINK novi);

/**
 * Function that makes a supervisor call.
 *
 * Supervisor call will only occur if ROUND_ROBIN scheduling is enbaled.
 */
extern void dispatch (void);

/**
 * Function that terminates task.
 *
 * When task reaches its end, this function is called so dispacher can be called 
 * and task will be finished correctly.
 */
extern void trmrq (void);

/**
 * Function to get handler of the running task.
 *
 * @return handler of currently running task
 */
extern int gethandler (void);

/**
 * Function that creates new task.
 *
 * Allocate memory needed for task and set initial parameters for task. Add task to list of descriptors.
 *
 * @param fun     task function
 * @param steklen stack length of the task
 * @param params  parameters of the task
 * @param tasktime TODO: Fix me
 * @return handler of created task
 */
extern int create_task ( void(*fun)(void*), int steklen, void* params, int tasktime);

/**
 * Function that kills task.
 *
 * Send message to SysMaintenance to kill the task if task wants to kill itself.
 * Otherwise clean memory. 
 *
 * @param task_handler handler of task
 * @return error code, 0 on successful
 */
extern int kill_task (int task_handler);

/**
 * Function that executes current task.
 */
extern void task_runner(void);

/**
 * Initialization function.
 *
 * Function that initializes the MT system. First create two tasks - SysMaintenance and IdleTask.
 * Initialize task that starts the system and replace interrupt handlers.
 *
 * @param mode scheduling mode
 * @param switch_rate TODO: Fix me
 *
 * @return error code, 0 on successful
 */
extern int initmt (int mode, int switch_rate);

/**
 * Function that aborts the system.
 *
 * Replace interrupt handlers with old ones.
 */
extern void abortmt (void);

/**
 * Exit the system.
 *
 * Abort the system, and clear task descriptors.
 */
extern void exitmt (void);

/**
 * Function that starts the task.
 *
 * This function has to prepare priority, state and context for starting task.
 * Task is then queued in ready tasks. Since a new task can be the highest priority,
 * call scheduler.
 *
 * @param task_handler handler of the task
 * @param priority priority that task will be run on
 * @return error code, 0 on successful
 */
extern int sptsk (int task_handler, int priority);

/**
 * Function that continues execution of the task.
 *
 * This function is called to continue execution of task that was blocked. 
 * Task is queued in ready tasks and scheduler is called.
 *
 * @param task_handler handler of the task
 * @return error code, 0 on successful
 */
extern int conttsk (int task_handler);

/**
 * Get parameters of the running task.
 *
 * @return parameters of the running task
 */
extern void* getparams( void );

/**
 * Function that initializes semaphore.
 *
 * @param sem pointer to sempahore
 * @param count initial semaphore count
 */
extern void dfsm (SEMPTR sem, int count);

/**
 * Function that executes P operation (wait) on sempahore.
 *
 * This function executes P operation (tries to substract 1 from semaphore). If semaphore value 
 * is already 0, task will be blocked until substractio nis possible.
 *
 * @param sem pointer to semaphore
 * @return bool indication if the task wasn't waiting
 */
extern int rsvsm (SEMPTR sem);

/**
 * Function that tries to executes P operation (wait) on sempahore.
 *
 * This function tries to executes P operation (tries to substract 1 from semaphore if the value of semaphore is greater than 1). If the semaphore value 
 * is already 0, task will continue without modifying the semaphore.
 *
 * @param sem pointer to semaphore
 * @return bool if the P operation was successful
 */
extern int tryrsvsm (SEMPTR sem);

/**
 * Function that executes V operation (post) on semaphore.
 *
 * This function executes V operation (adds 1 to semaphore). If there were blocked
 * tasks, one will be unblocked (queued in ready tasks).
 *
 * @param sem pointer to semaphore
 * @return bool indication if someone was waiting on semaphore
 */
extern int rlsm (SEMPTR sem);

/**
 * Function that executes V operation (post) on semaphore multiple times.
 *
 * @param sem pointer to semaphore
 * @param n number of times V operation is executed
 * @return bool indication if someone was waiting on semaphore
 */
extern int rlsm_count (SEMPTR sem, int n);

/**
 * Function that initializes atomic region.
 *
 * @param cs pointer to atomic region
 */
extern void init_critsec( CRIT_SECT* cs );

/**
 * Function that enters atomic region.
 *
 * @param cs pointer to atomic region
 * @return bool indication if someone was waiting to enter
 */
extern int enter_critsec( CRIT_SECT* cs );

/**
 * Function that checks if atomic region is locked.
 *
 * @param cs pointer to atomic region
 * @return bool indication if atomic region is locked
 */
extern int try_critsec( CRIT_SECT* cs );

/**
 * Function that leaves atomic region.
 *
 * @param cs pointer to atomic region
 * @return bool indication if someone was waiting to enter
 */
extern int leave_critsec( CRIT_SECT* cs );

/**
 * Suspend currently running task.
 *
 * This function suspends currently running task. State of task and 
 * cause of suspension are recorded and scheduler is called.
 *
 * @return -1 if suspension is not allowed
 */
extern int suspend (void);

/**
 * Delay task some time.
 *
 * TODO: Fix me
 *
 * @param dela_time amount of milliseconds to be delayed
 * @return -1 if delay did not succeed, otherwise 0
 */
extern int delay_task ( double delay_time );

/**
 * Request task to finish time suspension.
 *
 * TODO: Fix me
 *
 * @param lrn task handler
 * @return -1 if request did not succeed, otherwise 0
 */
extern int request (int lrn);

/**
 * Function to change priority of the running task.
 *
 * @param priority new priority of the task
 * @return -2 if priority is not in bounds, -1 if there is no running process, otherwise
 * previous priority
 */
extern int levelchange (int priority);

/**
 * Function that gets priority of currently running task.a.
 *
 * @return task's priority
 */
extern int levelget ( void );

/**
 * Change time switch of the runnng task.
 *
 * TODO: Fix me
 *
 * @param new_time new time that will be set
 * @return old time
 */
extern unsigned timechange (unsigned new_time);

/**
 * Get time of currently running task.
 *
 * TODO: Fix me
 *
 * @return time of running task
 */
extern unsigned timeget (void);

/**
 * Get tasks state.
 *
 * This function gets state and cause of suspension of currently running task.
 * Lowest 4 bits contain state and next 4 bits contain cause.
 *
 * @param task_handler task handler
 * @return state and cause of task 
 */
extern int gettaskstate (int task_handler);

/**
 * Disable time slice.
 *
 * This function disables scheduling on timer interrupt.
 *
 * @return 0 on success
 */
extern int timeslice_disable (void);

/**
 * Enable time slice.
 *
 * This function enables scheduling on timer interrupt.
 *
 * @return 0 on success
 */
extern int timeslice_enable (void);

/**
 * Set user interrupt 70h handler.
 *
 * @param user_int new user interrupt handler
 */
extern void set_user_int70 (void (*user_int)(void));

/**
 * Task mailbox creation.
 *
 * TODO: Fix me
 */
extern int create_task_mbox( int task, unsigned slots, unsigned datalen );

/**
 * Clear task mailbox
 *
 * TODO: Fix me
 */
extern int clear_task_mbox( int task );

/**
 * Deleate task mailbox
 *
 * TODO: Fix me
 */
extern int delete_task_mbox( int task );

/**
 * Deleate task mailbox
 *
 * TODO: Fix me
 */
extern int task_mbox_messages( int task );

/**
 * Send task message.
 *
 * TODO: Fix me
 */
extern int send_task_message( int task, int len, void* msg, int gde );

/**
 * Send task message.
 *
 * TODO: Fix me
 */
extern int send_cond_task_message( int task, int len, void* msg, int gde );

/**
 * Get task message
 *
 * TODO: Fix me
 */
extern int get_task_message( int len, void* msg );

/**
 * Get task message
 *
 * TODO: Fix me
 */
extern int get_cond_task_message( int len, void* msg );

/**
 * Read task message
 *
 * TODO: Fix me
 */
extern int read_task_message( int len, void* msg );

/**
 * Create mailbox
 *
 * TODO: Fix me 
 */
extern MBOX* create_mbox( unsigned slots, unsigned datalen );

/**
 * Delete mailbox
 *
 * TODO: Fix me 
 */
extern int delete_mbox( MBOX* mbp );

/**
 * Clear mailbox
 *
 * TODO: Fix me 
 */
extern int clear_mbox( MBOX* mbp );

/**
 *
 * TODO: Fix me 
 */
extern int mbox_messages( MBOX* mbp );

/**
 * Put message in mailbox
 *
 * TODO: Fix me 
 */
extern int put_message( MBOX* mbp, int len, void* msg, int gde );

/**
 *
 * TODO: Fix me 
 */
extern int put_cond_message( MBOX* mbp, int len, void* msg, int gde );

/**
 * Get message
 *
 * TODO: Fix me 
 */
extern int get_message( MBOX* mbp, int len, void* msg );

/**
 * Get message
 *
 * TODO: Fix me 
 */
extern int get_cond_message( MBOX* mbp, int len, void* msg );

/**
 * Read message
 *
 * TODO: Fix me 
 */
extern int read_message( MBOX* mbp, int len, void* msg );

/**
 * Function that initializes mutex.
 *
 * @param mtx pointer to mutex
 */
extern void dfmtx( MTXPTR mtx );

/**
 * Function that locks mutex.
 *
 * This function executes P operation on semaphore inside mutex.
 *
 * @param mtx pointer to mutex
 * @return bool indication if the task wasn waiting
 */
extern int rsvmtx( MTXPTR mtx );

/**
 * Function that unlocks mutex.
 *
 * This function executes V operation on semaphore inside mutex.
 *
 * @param mtx pokazivac na mutex
 * @return bool indication if some other task was waiting on mutex
 */
extern int rlsmtx( MTXPTR mtx );


/**
 *
 * TODO: Fix me
 *
 */
extern MTTIMERHND create_timer( unsigned time, void (*fun)(void*), void* param, int stacklen, int priority, int type );

/**
 *
 * TODO: Fix me
 *
 */
extern int start_timer( MTTIMERHND TimerHnd );

/**
 *
 * TODO: Fix me
 *
 */
extern int stop_timer( MTTIMERHND TimerHnd );

/**
 *
 * TODO: Fix me
 *
 */
extern int shot_timer( MTTIMERHND TimerHnd );

/**
 *
 * TODO: Fix me
 *
 */
extern int wait_on_timer( MTTIMERHND TimerHnd );

/**
 *
 * TODO: Fix me
 *
 */
extern int delete_timer( MTTIMERHND TimerHnd, int wait_on_timer );

/**
 * Kernel main.
 */
extern void kernel_main( uint32_t r0, uint32_t r1, uint32_t atags );

/**
 * Definition of assembly halt function.
 *
 * This function is just an infinite loop.
 */
extern void _halt(void);
/**
 * Definition of assembly take flags function.
 *
 * This function is used to get CPSR.
 *
 * @return CPSR
 */
extern int _take_flags(void);


#define alloc(x)		malloc(x)	/**< Macro for allocating memory*/
#define dealloc(x)	free(x)   		/**< Macro for clearing memory*/

/**
 * Macro for dispatcher call.
 *
 * This macro is just svc #0x60 assembly, which will cause supervisor call.
 * This will result in TaskDisp call, and task switching will be performed.
 */
#define calldisp()	asm("svc #0x60")

/**
 * Macro for saving CPSR.
 *
 * This macro is set of 5 assembly operations, which perform saving CPSR on stack.
 * First, we allocate space for CPSR on stack, then we save current r0.
 * Then we load CPSR to r0, place it on stack, and load back r0.
 */
#define push_flags() ({			\
	asm("sub	sp, sp, #4\n"		\
			"push {r0}\n"					\
			"mrs	r0, cpsr\n"			\
			"str	r0, [sp, #4]\n"	\
			"pop	{r0}":::"r0");					\
})

/**
 * Macro for retreive CPSR.
 *
 * This macro is set of 5 assembly operations, which perform retreiving of CPSR 
 * from stack. First we save current r0, and then load CPSR from stack to r0.
 * Then we load r0 back to CPSR, and retreive original r0.
 */
#define pop_flags()	({			\
	asm("push	{r0}\n"					\
			"ldr	r0, [sp, #4]\n" \
			"msr	cpsr, r0\n"			\
			"pop {r0}\n"					\
			"add	sp, sp, #4":::"r0");		\
})

#endif // KERNEL_H
