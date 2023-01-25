/**
 * Copyright Odsek RT-RK - free to use for educational purposes.
 *
 * RPI multitasking.
 *
 * This file contains all the multitasking functions and test functions.
 */


#include "kernel.h"
#include "rpi-gpio.h"
#include "rpi-armtimer.h"
#include "rpi-systimer.h"
#include "rpi-interrupts.h"
#include "rpi-uart.h"
#include "rpi-base.h"
#include "rpi-aux.h"
#include "types-mt.h"
#include "tasks.h"
#include "arm.h"
#include "stdlib.h"
#include <stdint.h>

#define MT_SYS_SLOTS          16
#define MT_SYS_SLOT_LEN       64

#define KILL_TASK 1

/**
 * TODO: Fix me
 */
void (*user_int70)(void);

static INTFPTR OldDispInt; /**< Function pointer to previous Supervisor call handler */

static INTFPTR OldTimeInt; /**< Function pointer to previous Timer interrupt handler */

/**
 * TODO:
 */
static unsigned int OldRMTimeInt;

/**
 * TODO:
 */
static unsigned int OldRMInt15;

LINK running       = NULL;  /**< Pointer to the current task descriptor */
LINK readyhead     = NULL;  /**< Pointer to the first ready task descriptor */
LINK readytail     = NULL;  /**< Pointer to the last ready task descriptoro */
LINK tim_susphead    = NULL;  /**< Pointer to the first time suspended task descriptor */

/**
 * Pointers to all task descriptors in the system.
 *
 * System is limited to LRNMAX tasks.
 * Program that started the system is also considered a task.
 * System initially creates two more tasks: System Maintaining task and Idle task.
 */
LINK LRT[LRNMAX];

MTTIMERHND TimerListHead;     /**< Pointer to the first Software timer */

static unsigned disp_mode;    /**< Scheduling mode of the system */

/**
 * Number of tasks in the system.
 *
 * Number of tasks is initially set to 1 but will be increased to 3 when 
 * System Maintaining task and Idle task are created.
 */
static unsigned task_no=1;

/**
 * Last created task index.
 *
 * Index of the last pointer in LRT that is not NULL. Some pointers before this one may be NULL
 */
static unsigned last_task=0;  

unsigned long int70_counter;   /**< Counter of timer interrupts for triggering user interrupt call */
unsigned int tick_counter;    /**< Counter of timer interrupts for running task  */

/**
 * An indicator of whether a switch is in progress.
 *
 * If TRUE, task switching is started and it should 
 * not be started again until finished.
 */
int in_switching;

int task_time;            /**< TODO: Fix me */
int mt_act;                   /**< An indicator of whether the system is initialized */
volatile int __change_task__; /**< An indicator of whether the system should change task */
static int time_susp;         /**< TODO: Fix me */
static int timer_count_susp;  /**< TODO: Fix me*/
static unsigned int ts_rate;  /**< Rate of task switching */
static int SysHnd;            /**< System handler */

volatile int ts_disabled;     /**< An indicator of whether task switching is disabled or enabled */

volatile int stop_timeslice;  /**< An indicator of whether time switching is disabled or enabled */

/**
 * TODO: Fix me
 */
#define stop_count()        ++time_susp

/**
 * TODO: Fix me
 */
#define start_count()       --time_susp

/**
 * TODO: Fix me
 */
#define stop_timer_handling()  ++timer_count_susp

/**
 * TODO: Fix me
 */
#define start_timer_handling() --timer_count_susp

/**
 * Macro that disables task switching
 */
#define switch_disable() ({     \
    push_flags();            \
    ++ts_disabled;          \
    pop_flags();            \
})

/**
 * Macro that enables task switching
 */
#define switch_enable() ({        \
    push_flags();            \
    --ts_disabled;          \
    pop_flags();            \
})

int set_disable(void){
  int flags;
  // Save previous CPSR
  flags = _take_flags();
  // Disable interrupts.
  disable();
  return flags;
}

void set_enable(int flags){
  // Check if interrupts were enabled.
  if(!(flags & ARM_I_BIT)){
    // Enable interrupts.
    enable();
  }
}

void queue(LINK novi){
  LINK p1,p2; 

  novi->next_ready = NULL;

  // If queue is empty.
  if( readyhead==NULL )
    readyhead = readytail = novi;
  else
  {
    p1 = p2 = readyhead;

    // Find task place.
    while( ( p2!=NULL ) && ( p2->I_PRIORITY<=novi->I_PRIORITY ) )
    {
      p1 = p2;
      p2 = p2->next_ready;
    }

    // If it is the end.
    if( p2==NULL )      
    {
      readytail->next_ready = novi;
      readytail = novi;
    }

    // If it is the beggining.
    if( p2==readyhead ) 
    {
      novi->next_ready = readyhead;
      readyhead  = novi;
    }
    // If it is in the middle.
    else       
    {
      p1->next_ready   = novi;
      novi->next_ready = p2;
    }
  }
}

void semqueue_by_priority ( LINK *S_QH, LINK* S_QT, LINK novi)
{
  LINK p1,p2;

   novi->next_sem_susp = NULL;
  // If queue is empty.
  if( *S_QH==NULL )
    *S_QH = *S_QT = novi;
  else
  {
    p1 = p2 = *S_QH;

    // Find task place.
    while( ( p2!=NULL ) && ( p2->I_PRIORITY<=novi->I_PRIORITY ) )
    {
      p1 = p2;
      p2 = p2->next_sem_susp;
    }

    // If it is the end.
    if( p2==NULL )
    {
      (*S_QT)->next_sem_susp = novi;
      *S_QT = novi;
    }

    // If it is the beggining.
    if( p2==*S_QH ) 
    {
      novi->next_sem_susp = *S_QH;
      *S_QH  = novi;
    }
    // If it is in the middle.
    else       
    {
      p1->next_sem_susp   = novi;
      novi->next_sem_susp = p2;
    }
  }
}

void dispatch (void)
{
   int flags, ret=0;

   flags=set_disable();
   // Change task.
   __change_task__=1;
   // Only if Round robin is enabled.
   if ( !(disp_mode&ROUND_ROBIN) )
      ret=1;
   set_enable(flags);

   if( !ret ) calldisp ();
}

void trmrq (void)
{
   int flags;
  
  if (running->I_LRN==0)
      return;

   flags=set_disable();
   running->I_STANJE = TASK_TERMINATED;
   set_enable(flags);
  while (running->I_STANJE == TASK_TERMINATED)
         if (disp_mode&ROUND_ROBIN)
            calldisp ();
}

int gethandler (void)
{
    int pom;

    if ( running )
       pom = running->I_LRN;
    else
       pom = -1;

    return(pom);
}

int create_task ( void(*fun)(void*), int steklen, void* params, int tasktime)
{
   int i, flags, task_handler;
   void*   stack_ptr;
   LINK    ISA_ptr;

   switch_disable();
   
   // TODO: Fix floating points
   //tasktime=(int)(tasktime*(0.9765625f)); // sracunato da ovoliko ima otkucaja sata
   tasktime = 0;

  
  // If there are too many tasks.
   if (task_no==LRNMAX)
      {
      switch_enable();
      return -2;
      }

    // Memory alloc for stack.
   if ((stack_ptr=alloc( steklen+CONTEXT_LEN))==NULL)
      {
      switch_enable();
      return -1;
      }

    // Memory alloc for descriptor.
   if ((ISA_ptr=(LINK)alloc( sizeof(ISA)))==NULL)
      {
      dealloc (stack_ptr);
      switch_enable();
      return(-1);
      }

    // Set descriptor up.
   ISA_ptr->stack_mem   = stack_ptr;
   ISA_ptr->start_point = fun;
   ISA_ptr->stack_len   = steklen;
   ISA_ptr->params      = params;

   ISA_ptr->I_STANJE    = TASK_TERMINATED;

   flags=set_disable();

   // Insert task descriptor into LRT
   task_no++;
   for( task_handler=0, i=1; i<last_task; i++ )
      if ( !LRT[i] )
         {
         task_handler=i;
         break;
         }
   if( !task_handler ) 
     task_handler=++last_task;
   ISA_ptr->I_LRN              = task_handler;
   LRT[task_handler]           = ISA_ptr;
   LRT[task_handler]->I_SWITCH = tasktime?tasktime:NO_TIME;
   LRT[task_handler]->I_PARENT = gethandler(); 

   set_enable(flags);
   switch_enable();

   return task_handler;
}

/**
 * Function that cleans up memory and removes its descriptor.
 * 
 * @param task_handler handler of task
 */
static void __destroy_task(int task_handler)
{
   switch_disable();
   // Dealloc stack area.
   dealloc ((void*)(LRT[task_handler]->stack_mem));

    // Dealloc descriptor.
   dealloc ((void*)LRT[task_handler]);
   LRT[task_handler]=NULL;
   --task_no;

    // Recalculate last task.
   if( task_handler==last_task )
     while( last_task>0 )
          {
          if ( LRT[last_task] )
             break;
          else
             last_task--;
          }

   switch_enable();
}

int kill_task (int task_handler)
{
   if (task_handler<=0 || task_handler>last_task || task_no==1)
      return -1;
   if (LRT[task_handler]==NULL)
      return -2;
   if (LRT[task_handler]->I_STANJE != TASK_TERMINATED)
   {
      // Task can't kill itself.
      if ( running==LRT[task_handler] )
      {
         int len=0;
         char buff[10];

         buff[0]=KILL_TASK;
         memcp( (char*)buff+1, (char*)&task_handler, len=sizeof( task_handler ) );
         
         send_task_message( SysHnd, ++len, buff, MAIL_ON_TAIL );
         trmrq(); 
      }
      return -3;
   }

   __destroy_task( task_handler );

   return 0;
}

/**
 * Dummy task function.
 *
 * Task that runs on lowest priority and does nothing.
 * @param dummy dummy argument
 */
static void IdleTask( void* dummy )
{
   dummy=dummy;
    while(1){};
}

/**
 * System task function.
 *
 * Task that runs on highest priority and takes care of 
 * self-killing processes.
 * @param dummy dummy argument
 */
static void SysMaintenance( void* dummy )
{
    int flags;

   BYTE buff[MT_SYS_SLOT_LEN];

   dummy=dummy;
   while(1)
   {
      get_task_message( MT_SYS_SLOT_LEN, buff );
      switch( buff[0] )
      {
         case KILL_TASK:
            {
               int TskHnd;
               memcp( (char*)&TskHnd, (char*)buff+1, sizeof( TskHnd ) );
               __destroy_task( TskHnd ); // Destroy task.
                
            }
            break;
      }
   }
}

void task_runner(void)
{
   int flags;
   void (*tsk_fun)(void*);

   flags=set_disable();
   tsk_fun=running->start_point;
   set_enable(flags);
   // Execute task.
   tsk_fun( running->params );
   
   trmrq();
}

int initmt (int mode, int switch_rate)
{
    int flags, IdleHnd;
    LINK    ISA_ptr;
 
    if( mt_act ) return( -1 );

    disp_mode = mode;

    if ((ISA_ptr=(LINK)alloc( sizeof(ISA)))==NULL)
       return(-1);

    IdleHnd = create_task ( IdleTask, 1000, NULL, NO_TIME );
    if ( IdleHnd<0 )
    {
       dealloc ((char*)ISA_ptr);
       return(-1);
    }

    SysHnd  = create_task ( SysMaintenance, 1000, NULL, NO_TIME );
    LRT[SysHnd]->mails.state = MBOX_DELETED;
    if ( SysHnd<0 )
    {
       __destroy_task( IdleHnd );
       dealloc ((char*)ISA_ptr);
       return(-1);
    }
   
    if (create_task_mbox( SysHnd, MT_SYS_SLOTS, MT_SYS_SLOT_LEN ))
    {
       __destroy_task( SysHnd );
       __destroy_task( IdleHnd );
       dealloc ((char*)ISA_ptr);
       return(-1);
    }    

    
    ISA_ptr->I_PRIORITY = PRMAX;
    ISA_ptr->I_STANJE   = TASK_READY;
    ISA_ptr->I_SWITCH   = NO_TIME;

    ISA_ptr->I_LRN = 0;
    LRT[0] = ISA_ptr;

    readyhead = NULL;
    readytail = NULL;
    running = ISA_ptr;

    OldDispInt = getvect (INT60); // Get address of old INT_DISP handler.    
    setvect (INT60, TaskDisp); // initialize interrupt INT_DISP vect. 



    if (mode & TIME_SLICE)
    {
      ts_rate=switch_rate;
    
      OldTimeInt = getvect (INT70);

      setvect(INT70, TimeInt);         
    }
    
    LRT[IdleHnd]->I_STANJE = TASK_BACKGND;
    LRT[IdleHnd]->I_PRIORITY = PRMAX+1; // Lowest priority.
    LRT[IdleHnd]->I_REGISTRI.cpsr = ARM_MODE_SYS|ARM_F_BIT;
     LRT[IdleHnd]->I_REGISTRI.pc = (uint32_t)task_runner;
     LRT[IdleHnd]->I_REGISTRI.sp = (unsigned)LRT[IdleHnd]->stack_mem
                                     + LRT[IdleHnd]->stack_len;
                                     
    queue (LRT[IdleHnd]);

    LRT[SysHnd]->I_STANJE = TASK_READY;
    LRT[SysHnd]->I_PRIORITY = PRMIN-1; // Highest priority.
    LRT[SysHnd]->I_REGISTRI.cpsr = ARM_MODE_SYS|ARM_F_BIT;
     LRT[SysHnd]->I_REGISTRI.pc = (uint32_t)task_runner;
     LRT[SysHnd]->I_REGISTRI.sp = (unsigned)LRT[SysHnd]->stack_mem
                                     + LRT[SysHnd]->stack_len;
                                     
    queue (LRT[SysHnd]);
    
    mt_act = 1;

    return(0);
}

void abortmt (void)
{
    int flags;

    /* Restore saved interrupt vectors. */

    if( !mt_act ) return;
    flags=set_disable();
    setvect (INT60, OldDispInt);
    
    // Restore old RTC settings and interrupt handlers.
    if (disp_mode&TIME_SLICE)
    {
      setvect (INT70, OldTimeInt);
    }
    mt_act=0;
    set_enable(flags);
}

void exitmt (void)
{
    int i;

    abortmt ();
    if( !mt_act ) return;

    // Destroy all tasks.
    for (i=1; i<=last_task; i++)
        if( LRT[i] )
          {
          LRT[i]->I_STANJE = TASK_TERMINATED;
          kill_task (i);
          }

    // Free MT task.
    dealloc ((void *)LRT[0]);
    LRT[0]=NULL;
    task_no =1;
    last_task=0;
}

int sptsk (int task_handler, int priority)
{

    int flags;

   if ((task_handler < LRNMIN) || (task_handler > last_task) || (task_no == 1)) return (1);
   if ( LRT[task_handler]==NULL) return (2);
   if (LRT[task_handler]->I_STANJE != TASK_TERMINATED) return (3);
   if ((priority < PRMIN) || (priority > PRMAX)) return (4);

   LRT[task_handler]->I_STANJE = TASK_READY;
   LRT[task_handler]->I_PRIORITY = priority;
   LRT[task_handler]->I_REGISTRI.cpsr = ARM_MODE_SYS|ARM_F_BIT;
   LRT[task_handler]->I_REGISTRI.pc = (uint32_t)task_runner;
   LRT[task_handler]->I_REGISTRI.sp = (unsigned)LRT[task_handler]->stack_mem
                                     + LRT[task_handler]->stack_len;

   flags=set_disable();
   queue (LRT[task_handler]);
   set_enable(flags);

   dispatch();
   return (0);
}

int conttsk (int task_handler)
{
   int flags;

   if ((task_handler < LRNMIN) || (task_handler > last_task) || task_no==1 ) return (1);
   if ( LRT[task_handler]==NULL) return (2);
   if (LRT[task_handler]->I_STANJE != TASK_TERMINATED) return (3);

   LRT[task_handler]->I_STANJE = TASK_READY;
   flags=set_disable();
   queue (LRT[task_handler]);
   set_enable(flags);

   dispatch();
   return (0);
}

void* getparams( void )
{
   if( running )
       return running->params;
   else
       return NULL;
}

void dfsm (SEMPTR sem, int count)
{
   sem->S_COUNT = count;
   sem->S_QH = sem->S_QT = NULL;
}

int rsvsm (SEMPTR sem)
{
   int  flags, pom=FALSE;
   LINK   suspptr;

   flags=set_disable ();
   --sem->S_COUNT;

  // Blocked on semaphore
   if (sem->S_COUNT < 0) 
      {
      suspptr = running;
      suspptr->next_sem_susp = NULL;

      if (sem->S_QH == NULL)
          sem->S_QH = sem->S_QT = suspptr;
      else
         {
         sem->S_QT->next_sem_susp = suspptr;
         sem->S_QT = suspptr;
         }

      running->I_UZROK  = SUSPEND_ON_SEM;
      running->I_STANJE = TASK_WAIT_SUSP;     // Waiting on suspension.

      set_enable(flags);
      while (running->I_STANJE == TASK_WAIT_SUSP)
            if (disp_mode&ROUND_ROBIN)
               calldisp ();
      set_disable();
      pom = TRUE;
      }
   set_enable (flags);
   return pom;
}

int tryrsvsm (SEMPTR sem)
{
   int  flags, pom=FALSE;
   LINK   suspptr;

   flags=set_disable ();
   if(sem->S_COUNT > 0){
     --sem->S_COUNT;
     pom = TRUE;
   }
   set_enable (flags);
   return pom;
}

int rlsm (SEMPTR sem)
{
   int flags, pom=FALSE;
   LINK suspptr;

   flags=set_disable ();
   ++sem->S_COUNT;
   // If there are some tasks blocked.
   if (sem->S_COUNT<=0 && sem->S_QH) 
      {
      suspptr = sem->S_QH;

      if ( suspptr->I_STANJE == TASK_SUSPENDED)
         {
         suspptr->I_STANJE = TASK_READY;
         queue ( suspptr );
         }
      else if ( suspptr->I_STANJE == TASK_WAIT_SUSP)
         suspptr->I_STANJE = TASK_READY;

      if (sem->S_QH == sem->S_QT)
          sem->S_QH = sem->S_QT = NULL;
      else
          sem->S_QH = sem->S_QH->next_sem_susp;

      if (disp_mode&ROUND_ROBIN)
         {
         set_enable (flags);
         dispatch();
         disable ();
         }
      pom = TRUE;
      }
   set_enable (flags);
   return pom;
}

int rlsm_count (SEMPTR sem, int n)
{
   int i, flags, pom=FALSE;
   LINK suspptr;

   for( i=0; i<n; i++ )
   {
      flags=set_disable ();
      ++sem->S_COUNT;
   // If there are some tasks blocked.
      if (sem->S_COUNT<=0 && sem->S_QH) 
         {
         suspptr = sem->S_QH;

         if ( suspptr->I_STANJE == TASK_SUSPENDED)
            {
            suspptr->I_STANJE = TASK_READY;
            queue ( suspptr );
            }
         else if ( suspptr->I_STANJE == TASK_WAIT_SUSP)
            suspptr->I_STANJE = TASK_READY;

         if (sem->S_QH == sem->S_QT)
             sem->S_QH = sem->S_QT = NULL;
         else
             sem->S_QH = sem->S_QH->next_sem_susp;

         if (disp_mode&ROUND_ROBIN)
            {
            set_enable (flags);
            dispatch();
            disable ();
            }
         pom = TRUE;
         }
      set_enable (flags);
   }
   return pom;
}

void init_critsec( CRIT_SECT* cs )
{
   cs->S_COUNT = 1;
   cs->S_QH=cs->S_QT = NULL;
}

int enter_critsec( CRIT_SECT* cs )
{
   return rsvsm( cs );
}

int try_critsec( CRIT_SECT* cs )
{
   int flags, pom=FALSE;

   flags=set_disable ();
   if ( cs->S_COUNT<=0 )
      pom = TRUE;
   set_enable (flags);

   return pom;
}

int leave_critsec( CRIT_SECT* cs )
{
   int flags, pom=FALSE;
   LINK suspptr;

   flags=set_disable ();
   if( cs->S_COUNT<1 )
   {
      ++cs->S_COUNT;
   // If there are some tasks blocked.
      if (cs->S_COUNT <= 0 && cs->S_QH) 
         {
         suspptr = cs->S_QH;

         if ( suspptr->I_STANJE == TASK_SUSPENDED)
            {
            suspptr->I_STANJE = TASK_READY;
            queue ( suspptr );
            }
         else if ( suspptr->I_STANJE == TASK_WAIT_SUSP)
            suspptr->I_STANJE = TASK_READY;

         if (cs->S_QH == cs->S_QT)
             cs->S_QH = cs->S_QT = NULL;
         else
             cs->S_QH = cs->S_QH->next_sem_susp;

         if (disp_mode&ROUND_ROBIN)
            {
            set_enable (flags);
            dispatch();
            disable ();
            }
         pom = TRUE;
         }
      set_enable (flags);
   }
   set_enable (flags);

   return pom;
}

int suspend (void)
{
    if (!(disp_mode&TIME_SLICE))
       return -1;

    switch_disable ();
    running->I_UZROK=PERM_SUSPEND;
    running->I_STANJE=TASK_WAIT_SUSP;
    switch_enable ();

    while (running->I_STANJE==TASK_WAIT_SUSP)
          if (disp_mode&ROUND_ROBIN)
             calldisp ();

    return 0;
}

int delay_task ( double delay_time )
{
    unsigned long time;

    if (!(disp_mode&TIME_SLICE))
       return -1;

    if (delay_time<0)
       return -1;

    switch_disable ();
  // TODO: Fix time
    time= 0; //(unsigned)(delay_time*(0.9765625f));
    time=time?time:1;

    if (delay_time)
       {
       LINK temp1, temp2;

       stop_count (); 
       //  If list is not empty.
       if ( tim_susphead )  
          {
          temp1=temp2=tim_susphead;

          // Find place.
          while(temp2!=NULL && temp2->I_VREME<=time)
               {
               temp1=temp2;
               temp2=temp2->next_tim_susp;
               time -= temp1->I_VREME;
               }

          // At the beggining.
          if (temp2==tim_susphead) 
             tim_susphead=running;
          else
             temp1->next_tim_susp=running;
          running->I_VREME=time;
          running->next_tim_susp=temp2;

          if (temp2 && time)
             temp2->I_VREME-=time;
          }
       // List is empty.
       else 
          {
          tim_susphead=running;  
          tim_susphead->next_tim_susp=NULL;
          tim_susphead->I_VREME=time;
          }
       running->I_UZROK=TEMP_SUSPEND;
       start_count (); 
       }
    else
       running->I_UZROK=PERM_SUSPEND;
    running->I_STANJE=TASK_WAIT_SUSP;
    switch_enable ();

    while (running->I_STANJE==TASK_WAIT_SUSP)
          if (disp_mode&ROUND_ROBIN)
             calldisp ();

    return 0;
}

int request (int lrn)
{
   int flags;
   LINK prevsusp, suspptr;

   if (LRT[lrn]->I_STANJE != TASK_SUSPENDED ||
        (LRT[lrn]->I_UZROK  != PERM_SUSPEND &&
         LRT[lrn]->I_UZROK  != TEMP_SUSPEND
        )
      ) return (1);

    switch_disable();
    if (LRT[lrn]->I_UZROK  == TEMP_SUSPEND)
       {
       stop_count (); 
       prevsusp=NULL;
       suspptr=tim_susphead;
       while (suspptr!=NULL && suspptr->I_LRN != lrn)
             {
             prevsusp=suspptr;
             suspptr=suspptr->next_tim_susp;
             }
       if (!suspptr)
          {
          start_count ();
          switch_enable();
          return -1;
          }
       if (suspptr->next_tim_susp && suspptr->I_VREME)
          suspptr->next_tim_susp->I_VREME+=suspptr->I_VREME;
       if (prevsusp)
          prevsusp->next_tim_susp=suspptr->next_tim_susp;
       else
          tim_susphead=suspptr->next_tim_susp;
       start_count ();
       }
    LRT[lrn]->I_STANJE = TASK_READY;

    flags=set_disable();
    disable();
    queue (LRT[lrn]);
    set_enable(flags);

    switch_enable();

    dispatch();

    return 0;
}

int levelchange (int priority)
{
   int pom, flags;

   if( priority>PRMAX || priority<PRMIN ) return -2;

   flags=set_disable();
   if ( running )
      {
      pom = running->I_PRIORITY;
      running->I_PRIORITY = priority;
      }
   else
      pom=-1;
   set_enable(flags);
   if( pom!=-1 )
     dispatch();
   return(pom);
}

int levelget ( void )
{
   int pom, flags;

   flags=set_disable();
   if (running)
      pom = running->I_PRIORITY;
   else
      pom = -1;
   set_enable(flags);
   return(pom);
}

unsigned timechange (unsigned new_time)
{
    unsigned pom, flags;

    flags=set_disable();
    // TODO: Fix time
    new_time=0;//(unsigned)(new_time*(0.9765625f));
    pom = running->I_SWITCH;
    running->I_SWITCH = new_time;
    set_enable(flags);

    return (pom);
}

unsigned timeget (void)
{
    int pom, flags;

    flags=set_disable();
    // TODO: Fix time
    pom = 0;//(int)(running->I_SWITCH/(0.9765625f));
    set_enable(flags);
    return(pom);
}

int gettaskstate (int task_handler)
{
    int pom, flags;

    if (task_handler<0 || task_handler>last_task || task_no==1 || LRT[task_handler]==NULL)
       return TASK_NOT_CREATED;

    flags=set_disable();
    pom = LRT[task_handler]->I_STANJE | (LRT[task_handler]->I_UZROK<<4);
    set_enable(flags);
    return (pom);
}

int timeslice_disable (void)
{
   int flags;

   if (disp_mode&TIME_SLICE)
      {
      flags=set_disable ();
      ++stop_timeslice;
      set_enable (flags);
      return 0;
      }

   return -1;
}

int timeslice_enable (void)
{
   int flags;

   if ((disp_mode&TIME_SLICE) && stop_timeslice)
      {
      flags=set_disable ();
      --stop_timeslice;
      set_enable (flags);
      return 0;
      }

   return -1;
}

void TaskDisp (struct STACK_FRAME * reg)
{
  int flags;
    // Check if switching is in progress.
     if (in_switching)
        return;
     in_switching=TRUE;


     if (!ts_disabled)  
       {
       if (readyhead &&
           ((running->I_PRIORITY >= readyhead->I_PRIORITY) ||
            (running->I_STANJE == TASK_WAIT_SUSP) ||
            (running->I_STANJE == TASK_TERMINATED)
           )
          ){
          // Context switching.
          running->I_REGISTRI.sp=reg->sp;
          running->I_REGISTRI.lr=reg->lr;

          running->I_REGISTRI.r0=reg->r0;
          running->I_REGISTRI.r1=reg->r1;
          running->I_REGISTRI.r2=reg->r2;
          running->I_REGISTRI.r3=reg->r3;

          running->I_REGISTRI.r4=reg->r4;
          running->I_REGISTRI.r5=reg->r5;
          running->I_REGISTRI.r6=reg->r6;
          running->I_REGISTRI.r7=reg->r7;
          
          running->I_REGISTRI.r8=reg->r8;
          running->I_REGISTRI.r9=reg->r9;
          running->I_REGISTRI.r10=reg->r10;
          
          running->I_REGISTRI.fp=reg->fp;
          running->I_REGISTRI.ip=reg->ip;
          running->I_REGISTRI.pc=reg->pc;
          running->I_REGISTRI.cpsr=reg->cpsr;

          if( int70_counter>=running->start_time )
             running->total_time_low += int70_counter - running->start_time;
          else
             running->total_time_low += int70_counter +(0xFFFFFFFFUL-running->start_time);

          running->total_time_hi += running->total_time_low/1000;
          running->total_time_low = running->total_time_low%1000;

          // Suspend task
          if (running->I_STANJE == TASK_WAIT_SUSP)
             running->I_STANJE = TASK_SUSPENDED;
          // Put task in ready tasks.
          else if (running->I_STANJE != TASK_TERMINATED)
             queue (running); 

          if (readyhead==readytail)
             readytail=NULL;
          running = readyhead;
          readyhead = readyhead->next_ready;
          running->next_ready = NULL;

          reg->sp=running->I_REGISTRI.sp;
          reg->lr=running->I_REGISTRI.lr;

          reg->r0=running->I_REGISTRI.r0;
          reg->r1=running->I_REGISTRI.r1;
          reg->r2=running->I_REGISTRI.r2;
          reg->r3=running->I_REGISTRI.r3;

          reg->r4=running->I_REGISTRI.r4;
          reg->r5=running->I_REGISTRI.r5;
          reg->r6=running->I_REGISTRI.r6;
          reg->r7=running->I_REGISTRI.r7;
          
          reg->r8=running->I_REGISTRI.r8;
          reg->r9=running->I_REGISTRI.r9;
          reg->r10=running->I_REGISTRI.r10;
          
          reg->fp=running->I_REGISTRI.fp;
          reg->ip=running->I_REGISTRI.ip;
          reg->pc=running->I_REGISTRI.pc;
          reg->cpsr=running->I_REGISTRI.cpsr;
          
          running->start_time = int70_counter;
          
          tick_counter = 0;
          
          }
          
          __change_task__ = 0;
       }
     in_switching=FALSE;
}

void set_user_int70 (void (*user_int)(void))
{
   switch_disable();
   user_int70=user_int;
   switch_enable();
}

void TimeInt (struct STACK_FRAME * reg)
{
  MTTIMERHND CurrTimer;
  
  int70_counter++;
  
  if (user_int70!=NULL)
        {
        switch_disable();
        user_int70 ();
        switch_enable();
        }

  if (ts_rate!=NO_TIME && tick_counter<ts_rate)
        ++tick_counter;
        
  if (!time_susp && tim_susphead)
         
        if (--(tim_susphead->I_VREME)==0)
           {
           while (tim_susphead && tim_susphead->I_VREME==0)
                 {
                 tim_susphead->I_STANJE=TASK_READY;
                 queue (LRT[tim_susphead->I_LRN]);
                 tim_susphead=tim_susphead->next_tim_susp;
                 }
           __change_task__=1;
           }
           

     if (!timer_count_susp && TimerListHead)
        if (--(TimerListHead->deltaT)==0)
        {
           while (TimerListHead && TimerListHead->deltaT==0)
           {
               CurrTimer = TimerListHead;
               TimerListHead=TimerListHead->next_timer;

               rlsm_count( &(CurrTimer->timer_sem), CurrTimer->timer_triger );
               if( CurrTimer->timer_fun_handler )
                  sptsk( CurrTimer->timer_fun_handler, CurrTimer->timer_fun_priority );
               if( CurrTimer->timer_type&MTTIMER_CYCLIC )
               {
                  CurrTimer->timer_status=MTTIMER_INACTIVE;
                  start_timer( CurrTimer );
               }
           }
           __change_task__=1;
        }

     if (task_time && task_time!=NO_TIME) --task_time;

  
    // Check if switching is in progress.
     if (in_switching)
        return;
     in_switching=TRUE;


     if (!ts_disabled && 
         !stop_timeslice &&
         (tick_counter==ts_rate || task_time==0 || __change_task__))  
       {
       if (readyhead &&
           ((running->I_PRIORITY >= readyhead->I_PRIORITY) ||
            (running->I_STANJE == TASK_WAIT_SUSP) ||
            (running->I_STANJE == TASK_TERMINATED)
           )
          ){
        
          // Context switching.
          running->I_REGISTRI.sp=reg->sp;
          running->I_REGISTRI.lr=reg->lr;

          running->I_REGISTRI.r0=reg->r0;
          running->I_REGISTRI.r1=reg->r1;
          running->I_REGISTRI.r2=reg->r2;
          running->I_REGISTRI.r3=reg->r3;

          running->I_REGISTRI.r4=reg->r4;
          running->I_REGISTRI.r5=reg->r5;
          running->I_REGISTRI.r6=reg->r6;
          running->I_REGISTRI.r7=reg->r7;
          
          running->I_REGISTRI.r8=reg->r8;
          running->I_REGISTRI.r9=reg->r9;
          running->I_REGISTRI.r10=reg->r10;
          
          running->I_REGISTRI.fp=reg->fp;
          running->I_REGISTRI.ip=reg->ip;
          running->I_REGISTRI.pc=reg->pc;
          running->I_REGISTRI.cpsr=reg->cpsr;

          if( int70_counter>=running->start_time )
             running->total_time_low += int70_counter - running->start_time;
          else
             running->total_time_low += int70_counter +(0xFFFFFFFFUL-running->start_time);

          running->total_time_hi += running->total_time_low/1000;
          running->total_time_low = running->total_time_low%1000;
          
          // Suspend task
          if (running->I_STANJE == TASK_WAIT_SUSP){
             running->I_STANJE = TASK_SUSPENDED;
           }
          // Put task in ready tasks.
          else if (running->I_STANJE != TASK_TERMINATED){
             queue (running); 
          }

          if (readyhead==readytail){
             readytail=NULL;
          }
          running = readyhead;
          readyhead = readyhead->next_ready;
          running->next_ready = NULL;

          reg->sp=running->I_REGISTRI.sp;
          reg->lr=running->I_REGISTRI.lr;

          reg->r0=running->I_REGISTRI.r0;
          reg->r1=running->I_REGISTRI.r1;
          reg->r2=running->I_REGISTRI.r2;
          reg->r3=running->I_REGISTRI.r3;

          reg->r4=running->I_REGISTRI.r4;
          reg->r5=running->I_REGISTRI.r5;
          reg->r6=running->I_REGISTRI.r6;
          reg->r7=running->I_REGISTRI.r7;
          
          reg->r8=running->I_REGISTRI.r8;
          reg->r9=running->I_REGISTRI.r9;
          reg->r10=running->I_REGISTRI.r10;
          
          reg->fp=running->I_REGISTRI.fp;
          reg->ip=running->I_REGISTRI.ip;
          reg->pc=running->I_REGISTRI.pc;
          reg->cpsr=running->I_REGISTRI.cpsr;
          
          running->start_time = int70_counter;
          
          tick_counter = 0;
          task_time = running->I_SWITCH;          
          }
          
          __change_task__ = 0;
       }
     in_switching=FALSE;
}

int create_task_mbox( int task, unsigned slots, unsigned datalen )
{
   int i, flags, ret = 0;
   MBOX* mbptr;
   char* ptr;

   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (1);
   if ( LRT[task]==NULL) return (2);
   flags=set_disable();
   if ( LRT[task]->mails.state!=MBOX_DELETED )
      {
      ret=3;
      goto izlaz;
      }

   mbptr=&(LRT[task]->mails);
   memse((char*) mbptr, 0, sizeof( MBOX ) );

  // Alloc all needed memory.
   if ( (ptr=alloc( slots*sizeof(char*)+slots*datalen+2*sizeof( SEM ) ))==NULL )
      {
      ret=4;
      goto izlaz;
      }

   mbptr->mailboxp = (char**)ptr;
   ptr+=slots*sizeof(char*);

   for( i=0; i<slots; i++, ptr+=datalen )
      mbptr->mailboxp[i]=ptr;

   mbptr->smPutMsg = ptr;
   ptr+=sizeof(SEM);

   mbptr->smGetMsg = ptr;

   mbptr->slots = slots;
   mbptr->msglen = datalen;

   dfsm( (SEMPTR)(mbptr->smPutMsg), slots );
   dfsm( (SEMPTR)(mbptr->smGetMsg), 0 );
   mbptr->state=MBOX_EMPTY;

izlaz:
   set_enable(flags);

   return ret;
}

int clear_task_mbox( int task )
{
   int flags, ret=0;
   MBOX* mbptr;

   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (1);
   if ( LRT[task]==NULL) return (2);

   flags=set_disable();
   if ( LRT[task]->mails.state==MBOX_INUSE )
      {
      ret = 5;
      goto izlaz;
      }

   if ( LRT[task]->mails.state==MBOX_DELETED )
      {
      ret = 3;
      goto izlaz;
      }

   mbptr=&(LRT[task]->mails);
   if (((SEMPTR)(mbptr->smPutMsg))->S_QH ||
       ((SEMPTR)(mbptr->smGetMsg))->S_QH)
      {
      ret = 4;
      goto izlaz;
      }

   mbptr->head=mbptr->tail=mbptr->count=0;
   mbptr->state=MBOX_EMPTY;

izlaz:
   set_enable(flags);

   return ret;
}

int delete_task_mbox( int task )
{
   MBOX* mbptr;
   int flags, ret = 0;

   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (1);
   if ( LRT[task]==NULL) return (2);

   flags=set_disable();

   if ( LRT[task]->mails.state==MBOX_DELETED )
      {
      ret = 3;
      goto izlaz;
      }

   if ( LRT[task]->mails.state==MBOX_INUSE )
      {
      ret = 5;
      goto izlaz;
      }


   mbptr=&(LRT[task]->mails);
   if (((SEMPTR)(mbptr->smPutMsg))->S_QH ||
       ((SEMPTR)(mbptr->smGetMsg))->S_QH )
      {
      ret=4;
      goto izlaz;
      }

   dealloc( mbptr->mailboxp );
   memse( (char*)mbptr, 0, sizeof( MBOX ) );
   mbptr->state=MBOX_DELETED;

izlaz:
   set_enable(flags);

   return ret;
}

int task_mbox_messages( int task )
{
   int flags, ret;
   MBOX* mbptr;

   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (-1);
   if ( LRT[task]==NULL) return (-2);

   flags=set_disable();

   if ( LRT[task]->mails.state==MBOX_DELETED )
      {
      ret = -3;
      goto izlaz;
      }

   mbptr=&(LRT[task]->mails);
   ret = mbptr->count;

izlaz:
   set_enable(flags);

   return ret;
}

int send_task_message( int task, int len, void* msg, int gde )
{
   int curstate, flags, slotoff;
   MBOX* mbptr;

   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (-1);
   if ( LRT[task]==NULL) return (-2);

   flags=set_disable();
   if ( (curstate=LRT[task]->mails.state)==MBOX_DELETED )
      {
      set_enable(flags);
      return -3;
      }
   LRT[task]->mails.state=MBOX_INUSE;
   set_enable(flags);

   mbptr=&(LRT[task]->mails);
   if( len>mbptr->msglen )
     len=mbptr->msglen;

   rsvsm( (SEMPTR)(mbptr->smPutMsg) );

   flags=set_disable();
   if ( gde == MAIL_ON_TAIL )
      {
      slotoff=mbptr->tail;
      mbptr->tail = (slotoff+ 1) % mbptr->slots;
      }
   else //  MAIL_ON_HEAD
      {
      if( mbptr->head )
         mbptr->head--;
      else
         mbptr->head = mbptr->slots - 1;
      slotoff=mbptr->head;
      }
   memcp( (char*)mbptr->mailboxp[slotoff], (char*)msg, len );
   mbptr->count++;
   set_enable(flags);
   LRT[task]->mails.state=curstate;
   rlsm( (SEMPTR)(mbptr->smGetMsg) );

   return len;
}

int send_cond_task_message( int task, int len, void* msg, int gde )
{
   int flags, ret, slotoff;
   MBOX* mbptr;

   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (-1);
   if ( LRT[task]==NULL) return (-2);

   flags=set_disable();
   if ( LRT[task]->mails.state==MBOX_DELETED )
     {
     ret = -3;
     goto izlaz;
     }

   mbptr=&(LRT[task]->mails);

   if( mbptr->count==mbptr->slots )
     {
     ret = -4;
     goto izlaz;
     }

   if( len>mbptr->msglen )
     len=mbptr->msglen;

   rsvsm( (SEMPTR)(mbptr->smPutMsg) );

   if ( gde == MAIL_ON_TAIL )
      {
      slotoff=mbptr->tail;
      mbptr->tail = (slotoff+ 1) % mbptr->slots;
      }
   else //  MAIL_ON_HEAD 
      {
      if( mbptr->head )
         mbptr->head--;
      else
         mbptr->head = mbptr->slots - 1;
      slotoff=mbptr->head;
      }
   memcp( (char*)mbptr->mailboxp[slotoff], (char*)msg, len );
   mbptr->count++;
   ret = len;

izlaz:
   set_enable( flags );

   rlsm( (SEMPTR)(mbptr->smGetMsg) );

   return ret;
}

int get_task_message( int len, void* msg )
{
   int flags, curstate, task;
   MBOX* mbptr;

   task=gethandler();

   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (-1);
   if ( LRT[task]==NULL) return (-2);

   flags=set_disable();
   if ( (curstate=LRT[task]->mails.state)==MBOX_DELETED )
      {
      set_enable(flags);
      return -3;
      }
   LRT[task]->mails.state=MBOX_INUSE;
   set_enable(flags);

   mbptr=&(LRT[task]->mails);

   if( len>mbptr->msglen )
     len=mbptr->msglen;

   rsvsm( (SEMPTR)(mbptr->smGetMsg) );

   flags=set_disable();
   memcp( (char*)msg, (char*)mbptr->mailboxp[mbptr->head], len );
   mbptr->head = (mbptr->head + 1) % mbptr->slots;
   mbptr->count--;
   LRT[task]->mails.state=curstate;
   set_enable(flags);

   rlsm( (SEMPTR)(mbptr->smPutMsg) );

   return len;
}

int get_cond_task_message( int len, void* msg )
{
   int flags, ret, task;
   MBOX* mbptr;

   task=gethandler();
   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (-1);
   if ( LRT[task]==NULL) return (-2);

   flags=set_disable();
   if ( LRT[task]->mails.state==MBOX_DELETED )
     {
     ret = -3;
     goto izlaz;
     }

   mbptr=&(LRT[task]->mails);

   if( mbptr->count==0 )
     {
     ret = -4;
     goto izlaz;
     }

   if( len>mbptr->msglen )
     len=mbptr->msglen;

   rsvsm( (SEMPTR)(mbptr->smGetMsg) );
   memcp( (char*)msg, (char*)mbptr->mailboxp[mbptr->head], len );
   mbptr->head = (mbptr->head + 1) % mbptr->slots;
   mbptr->count--;
   ret = len;

izlaz:
   set_enable( flags );

   rlsm( (SEMPTR)(mbptr->smGetMsg) );

   return ret;
}

int read_task_message( int len, void* msg )
{
   int ret, flags, task;
   MBOX* mbptr;

   task=gethandler();
   if ((task < LRNMIN) || (task > last_task) || task_no==1) return (-1);
   if ( LRT[task]==NULL) return (-2);

   flags=set_disable();
   if ( LRT[task]->mails.state!=MBOX_DELETED )
      {
      mbptr=&(LRT[task]->mails);

      if( len>mbptr->msglen )
        len=mbptr->msglen;

      if ( mbptr->count )
         {
         memcp( (char*)msg, (char*)mbptr->mailboxp[mbptr->head], len );
         ret = len;
         }
      else
         ret = -4;
      }
   else
      ret = -3;
   set_enable(flags);

   return ret;
}

MBOX* create_mbox( unsigned slots, unsigned datalen )
{
   int i;
   MBOX* mbp;
   char* ptr;

   switch_disable();
   if ( (ptr=alloc( sizeof(MBOX)+slots*sizeof(char*)+slots*datalen+2*sizeof( SEM ) ))==NULL )
      {
      switch_enable();
      mbp = NULL;
      goto izlaz;
      }
   switch_enable();

   mbp = (MBOX*)ptr;
   ptr+=sizeof(MBOX);

   mbp->mailboxp = (char**)ptr;
   ptr+=slots*sizeof(char*);

   for( i=0; i<slots; i++, ptr+=datalen )
      mbp->mailboxp[i]=ptr;

   mbp->smPutMsg = ptr;
   ptr+=sizeof(SEM);

   mbp->smGetMsg = ptr;

   mbp->slots = slots;
   mbp->msglen = datalen;

   dfsm( (SEMPTR)(mbp->smPutMsg), slots );
   dfsm( (SEMPTR)(mbp->smGetMsg), 0 );
   mbp->state=MBOX_EMPTY;
izlaz:

   return mbp;
}

int delete_mbox( MBOX* mbp )
{
   int flags, ret = 0;

   flags=set_disable();

   if ( mbp->state==MBOX_DELETED )
      {
      ret = -1;
      goto izlaz;
      }

   if ( mbp->state==MBOX_INUSE )
      {
      ret = -2;
      goto izlaz;
      }


   if (((SEMPTR)(mbp->smPutMsg))->S_QH ||
       ((SEMPTR)(mbp->smGetMsg))->S_QH )
      {
      ret = -3;
      goto izlaz;
      }

   dealloc( mbp );

izlaz:
   set_enable(flags);

   return ret;
}

int clear_mbox( MBOX* mbp )
{
   int flags, ret=0;

   flags=set_disable();
   if ( mbp->state==MBOX_INUSE )
      {
      ret = -1;
      goto izlaz;
      }

   if ( mbp->state==MBOX_DELETED )
      {
      ret = -2;
      goto izlaz;
      }

   if (((SEMPTR)(mbp->smPutMsg))->S_QH ||
       ((SEMPTR)(mbp->smGetMsg))->S_QH)
      {
      ret = -3;
      goto izlaz;
      }

   mbp->head=mbp->tail=mbp->count=0;
   mbp->state=MBOX_EMPTY;

izlaz:
   set_enable(flags);

   return ret;
}

int mbox_messages( MBOX* mbp )
{
   int flags, ret;

   flags=set_disable();
   if ( mbp->state==MBOX_DELETED )
      {
      ret = -1;
      goto izlaz;
      }

   ret = mbp->count;

izlaz:
   set_enable(flags);

   return ret;
}

int put_message( MBOX* mbp, int len, void* msg, int gde )
{
   int curstate, flags, slotoff;

   flags=set_disable();
   if ( (curstate=mbp->state)==MBOX_DELETED )
      {
      set_enable(flags);
      return -1;
      }
   mbp->state=MBOX_INUSE;
   set_enable(flags);

   if( len>mbp->msglen )
     len=mbp->msglen;

   rsvsm( (SEMPTR)(mbp->smPutMsg) );

   flags=set_disable();
   if ( gde == MAIL_ON_TAIL )
      {
      slotoff=mbp->tail;
      mbp->tail = (slotoff+ 1) % mbp->slots;
      }
   else /*  MAIL_ON_HEAD */
      {
      if( mbp->head )
         mbp->head--;
      else
         mbp->head = mbp->slots - 1;
      slotoff=mbp->head;
      }
   memcp( (char*)mbp->mailboxp[slotoff], (char*)msg, len );
   mbp->count++;

   if (!((SEMPTR)(mbp->smPutMsg))->S_QH &&
       !((SEMPTR)(mbp->smGetMsg))->S_QH ) 
      // If there are no blocked tasks on mailbox, retain starting state.
      mbp->state=curstate; 


   set_enable(flags);
   rlsm( (SEMPTR)(mbp->smGetMsg) );

   return len;
}

int put_cond_message( MBOX* mbp, int len, void* msg, int gde )
{
   int flags, ret, slotoff;

   flags=set_disable();
   if ( mbp->state==MBOX_DELETED )
     {
     ret = -1;
     goto izlaz;
     }

   if( mbp->count==mbp->slots )
     {
     ret = -2;
     goto izlaz;
     }

   if( len>mbp->msglen )
     len=mbp->msglen;

   rsvsm( (SEMPTR)(mbp->smPutMsg) );

   if ( gde == MAIL_ON_TAIL )
      {
      slotoff=mbp->tail;
      mbp->tail = (slotoff+ 1) % mbp->slots;
      }
   else //  MAIL_ON_HEAD 
      {
      if( mbp->head )
         mbp->head--;
      else
         mbp->head = mbp->slots - 1;
      slotoff=mbp->head;
      }
   memcp( (char*)mbp->mailboxp[slotoff], (char*)msg, len );
   mbp->count++;
   ret = len;

izlaz:
   set_enable( flags );

   rlsm( (SEMPTR)(mbp->smGetMsg) );

   return ret;
}

int get_message( MBOX* mbp, int len, void* msg )
{
   int flags, curstate;

   flags=set_disable();
   if ( (curstate=mbp->state)==MBOX_DELETED )
      {
      set_enable(flags);
      return -1;
      }
   mbp->state=MBOX_INUSE;
   set_enable(flags);

   if( len>mbp->msglen )
     len=mbp->msglen;

   rsvsm( (SEMPTR)(mbp->smGetMsg) );

   flags=set_disable();
   memcp( (char*)msg, (char*)mbp->mailboxp[mbp->head], len );
   mbp->head = (mbp->head + 1) % mbp->slots;
   mbp->count--;

   if (!((SEMPTR)(mbp->smPutMsg))->S_QH &&
       !((SEMPTR)(mbp->smGetMsg))->S_QH ) 
      // If there are no blocked tasks on mailbox, retain starting state.
      mbp->state=curstate; 

   set_enable(flags);

   rlsm( (SEMPTR)(mbp->smPutMsg) );

   return len;
}

int get_cond_message( MBOX* mbp, int len, void* msg )
{
   int flags, ret;

   flags=set_disable();
   if( mbp->state==MBOX_DELETED )
     {
     ret = -1;
     goto izlaz;
     }

   if( mbp->count==0 )
     {
     ret = -2;
     goto izlaz;
     }

   if( len>mbp->msglen )
     len=mbp->msglen;

   rsvsm( (SEMPTR)(mbp->smGetMsg) );
   memcp( (char*)msg, (char*)mbp->mailboxp[mbp->head], len );
   mbp->head = (mbp->head + 1) % mbp->slots;
   mbp->count--;
   ret = len;

izlaz:
   set_enable( flags );

   rlsm( (SEMPTR)(mbp->smGetMsg) );

   return ret;
}

int read_message( MBOX* mbp, int len, void* msg )
{
   int ret, flags;

   flags=set_disable();
   if ( mbp->state!=MBOX_DELETED )
      {
      if( len>mbp->msglen )
        len=mbp->msglen;

      if ( mbp->count )
         {
         memcp( (char*)msg, (char*)mbp->mailboxp[mbp->head], len );
         ret = len;
         }
      else
         ret = -2;
      }
   else
      ret = -1;
   set_enable(flags);

   return ret;
}

void dfmtx( MTXPTR mtx )
{
   mtx->S_COUNT  = 1;
   mtx->S_OWNCNT = 0;
   mtx->S_OWNER  = mtx->S_QH = mtx->S_QT = NULL;
}

int rsvmtx( MTXPTR mtx )
{
   int  flags, pom=FALSE;
   LINK   suspptr;

   flags=set_disable ();

   if ( mtx->S_OWNER!=running )
      --mtx->S_COUNT;

   if ( mtx->S_COUNT<0 )
    // Blocked on mutex.
      { 
      suspptr = running;
      suspptr->next_sem_susp = NULL;

      if (mtx->S_QH == NULL)
          mtx->S_QH = mtx->S_QT = suspptr;
      else
         {
         mtx->S_QT->next_sem_susp = suspptr;
         mtx->S_QT = suspptr;
         }

      running->I_UZROK  = SUSPEND_ON_SEM;
      running->I_STANJE = TASK_WAIT_SUSP;     // Waiting on suspension. 

      set_enable(flags);
      while (running->I_STANJE == TASK_WAIT_SUSP)
            if (disp_mode&ROUND_ROBIN)
               calldisp ();
      set_disable();
      pom = TRUE;
      }

   ++mtx->S_OWNCNT;
   mtx->S_OWNER=running;

   set_enable (flags);
   return pom;
}

int rlsmtx( MTXPTR mtx )
{
   int flags, pom=FALSE;
   LINK suspptr;

   flags=set_disable ();
   if ( mtx->S_OWNER==running && mtx->S_COUNT<1 )
   {
      if ( mtx->S_OWNCNT ) --mtx->S_OWNCNT;

      if( !mtx->S_OWNCNT )
      {
         ++mtx->S_COUNT;
         // Check if there are blocked tasks.
         if (mtx->S_COUNT<=0 && mtx->S_QH) 
            {
            suspptr = mtx->S_QH;
            mtx->S_OWNER = suspptr;

            if ( suspptr->I_STANJE == TASK_SUSPENDED)
               {
               suspptr->I_STANJE = TASK_READY;
               queue ( suspptr );
               }
            else if ( suspptr->I_STANJE == TASK_WAIT_SUSP)
               suspptr->I_STANJE = TASK_READY;

            if (mtx->S_QH == mtx->S_QT)
                mtx->S_QH = mtx->S_QT = NULL;
            else
                mtx->S_QH = mtx->S_QH->next_sem_susp;

            if (disp_mode&ROUND_ROBIN)
               {
               set_enable (flags);
               dispatch();
               disable ();
               }
            pom = TRUE;
            }
         else
            mtx->S_OWNER = NULL;
      }
   }
   set_enable (flags);
   return pom;
}

MTTIMERHND create_timer( unsigned time, void (*fun)(void*), void* param, int stacklen, int priority, int type )
{
    MTTIMERHND TimerHnd=NULL;

    if (!(disp_mode&TIME_SLICE))
       return NULL;

    // TODO: Fix time
    time=0;//(unsigned)(time*(0.9765625f));
    if (time)
    {
       switch_disable();
       if( TimerHnd = alloc( sizeof( MTTIMER ) ) )
       {
           switch_enable();
           TimerHnd->cyclus             = time;
           TimerHnd->timer_type         = type;
           TimerHnd->timer_status       = MTTIMER_INACTIVE;
           TimerHnd->timer_fun_priority = priority;
           dfsm( &(TimerHnd->timer_sem), 0 );

           if( fun &&
               (TimerHnd->timer_fun_handler = create_task ( fun, stacklen, param, NO_TIME ) )<=0 )
           {
              switch_disable();
              dealloc( TimerHnd );
              switch_enable();
              TimerHnd=NULL;
           }

           if( TimerHnd && (type&MTTIMER_START_NOW) )
              start_timer( TimerHnd );
        }
        else
           switch_enable();
    }

    return TimerHnd;
}

int start_timer( MTTIMERHND TimerHnd )
{
   int ret = -1;
   unsigned time;
   MTTIMERHND PrevTimer, CurrTimer;

   if( TimerHnd && TimerHnd->timer_status==MTTIMER_INACTIVE )
   {
      time = TimerHnd->cyclus;

      stop_timer_handling();
      // If list is not empty.
      if( TimerListHead ) 
      {
         PrevTimer=CurrTimer=TimerListHead; 
         // Find position.

         while( CurrTimer!=NULL && CurrTimer->deltaT<=time)
         {
            PrevTimer=CurrTimer;
            CurrTimer=CurrTimer->next_timer;
            time -= PrevTimer->deltaT;
         }

         if ( CurrTimer==TimerListHead ) 
            TimerListHead=TimerHnd;
         else
            PrevTimer->next_timer=TimerHnd;

         TimerHnd->deltaT=time;
         TimerHnd->next_timer=CurrTimer;

         if (CurrTimer && time)
            CurrTimer->deltaT-=time;
      }
      // List is empty.
      else 
      {
         TimerListHead=TimerHnd; 
         TimerListHead->next_timer=NULL;
         TimerListHead->deltaT=time;
      }
      TimerHnd->timer_status = MTTIMER_ACTIVE;
      start_timer_handling();

      ret = 0;
   }

   return ret;
}

int stop_timer( MTTIMERHND TimerHnd )
{
   int ret=-1;
   MTTIMERHND PrevTimer, CurrTimer;

   switch_disable();
   if( TimerListHead && TimerHnd )
   {
       // Find timer.
       stop_timer_handling();
       PrevTimer=NULL;
       CurrTimer=TimerListHead;
       while( CurrTimer && CurrTimer!=TimerHnd )
       {
          PrevTimer=CurrTimer;
          CurrTimer=CurrTimer->next_timer;
       }

      // Remove timer if it is in list.
       if( CurrTimer==TimerHnd )
       {
          if ( CurrTimer->next_timer && CurrTimer->deltaT )
             CurrTimer->next_timer->deltaT+=CurrTimer->deltaT;
          if ( PrevTimer )
             PrevTimer->next_timer=CurrTimer->next_timer;
          else
             TimerListHead=CurrTimer->next_timer;

          TimerHnd->timer_status = MTTIMER_INACTIVE;
          rlsm_count( &(TimerHnd->timer_sem), TimerHnd->timer_triger );
          ret = 0;
       }

       start_timer_handling();
   }
   switch_enable();

   return ret;
}

int shot_timer( MTTIMERHND TimerHnd )
{
   int ret=-1;
   MTTIMERHND PrevTimer, CurrTimer=NULL;

   if( TimerHnd )
   {
      switch_disable();
      if( TimerListHead )
      {
       // Find timer.
          stop_timer_handling();
          PrevTimer=NULL;
          CurrTimer=TimerListHead;
          while( CurrTimer && CurrTimer!=TimerHnd )
          {
             PrevTimer=CurrTimer;
             CurrTimer=CurrTimer->next_timer;
          }

          // Remove timer if it is in list.
          if( CurrTimer==TimerHnd ) 
          {
             if ( CurrTimer->next_timer && CurrTimer->deltaT )
                CurrTimer->next_timer->deltaT+=CurrTimer->deltaT;
             if ( PrevTimer )
                PrevTimer->next_timer=CurrTimer->next_timer;
             else
                TimerListHead=CurrTimer->next_timer;

             TimerHnd->timer_status = MTTIMER_INACTIVE;
          }
          start_timer_handling();
      }

      // Release all tasks.
      rlsm_count( &(TimerHnd->timer_sem), TimerHnd->timer_triger );
      if( TimerHnd->timer_fun_handler )
         sptsk( TimerHnd->timer_fun_handler, TimerHnd->timer_fun_priority );

      // 
      if( CurrTimer==TimerHnd )
         if( TimerHnd->timer_type&MTTIMER_CYCLIC )
             start_timer( TimerHnd );

      switch_enable();

      ret = 0;
   }

   return ret;
}

int wait_on_timer( MTTIMERHND TimerHnd )
{
   int  flags, pom=FALSE;
   LINK   suspptr;
   SEMPTR sem;

   if( TimerHnd )
   {
      flags=set_disable ();

      TimerHnd->timer_triger++;
      sem = &TimerHnd->timer_sem;

      --sem->S_COUNT;

      if (sem->S_COUNT < 0) 
      // Blocked on semaphore.
         {
         suspptr = running;
         suspptr->next_sem_susp = NULL;

         if (sem->S_QH == NULL)
             sem->S_QH = sem->S_QT = suspptr;
         else
            {
            sem->S_QT->next_sem_susp = suspptr;
            sem->S_QT = suspptr;
            }

         running->I_UZROK  = SUSPEND_ON_SEM;
         running->I_STANJE = TASK_WAIT_SUSP;     // Waiting on suspension. 

         set_enable(flags);
         while (running->I_STANJE == TASK_WAIT_SUSP)
               if (disp_mode&ROUND_ROBIN)
                  calldisp ();
         set_disable();
         pom = TRUE;
         }
      set_enable (flags);
   }

   return pom;
}

int delete_timer( MTTIMERHND TimerHnd, int wait_on_timer )
{
   int hnd, ret=-1;

   switch_disable();
   if( TimerListHead && TimerHnd )
   {

       // Stom timer.
       ret = stop_timer( TimerHnd );

       hnd =  TimerHnd->timer_fun_handler;
       if ( hnd && LRT[hnd] ) // Check if timer task exists.
       {
          // Check if timer finished.
          if( LRT[hnd]->I_STANJE == TASK_TERMINATED )
          {
              dealloc( TimerHnd );
              kill_task( hnd );
          }
          else // Or it is still working.
          {
              if( wait_on_timer )
              {
                 while( LRT[hnd]->I_STANJE != TASK_TERMINATED )
                 {
                     switch_enable();
                     delay_task( 50 );
                     switch_disable();
                 }
                 dealloc( TimerHnd );
                 kill_task( hnd );
              }
              else
              {
                 ret = 1;
              }
          }
       }
       else
          dealloc( TimerHnd );
   }
   switch_enable();

   return ret;
}

/**
 * Shared memmory
 */
#define SHMEM_SIGNATURE "SHMBLK1"
#define SHMEM_SIGNLEN   8
typedef struct shmem_header
{
  char shmem_signature[SHMEM_SIGNLEN];
  struct linked_list* S_QH;
  struct linked_list* S_QT;
  unsigned char shmem_status;
  unsigned int readin_counter;
} SHMEM_HEADER;
#define SHMEM_HDRLEN sizeof( SHMEM_HEADER )

#define SHMEM_FREE  0x01
#define SHMEM_READ  0x02
#define SHMEM_WRITE 0x04
#define SHMEM_FIFO  0x08
#define SHMEM_FRREQ 0x10


void* shmem_alloc( unsigned int size, int fifo )
{
  void* shmem_ptr;
  SHMEM_HEADER* shmem_hdrp;

  shmem_hdrp = (SHMEM_HEADER*)alloc( SHMEM_HDRLEN+size );
  shmem_ptr  = (char*)shmem_hdrp+SHMEM_HDRLEN;
  
  shmem_hdrp->S_QH=shmem_hdrp->S_QT=0;
  shmem_hdrp->shmem_status = SHMEM_FREE;
  if( fifo ) shmem_hdrp->shmem_status |= SHMEM_FIFO;
  shmem_hdrp->readin_counter = 0;
  memcp( (char*)shmem_hdrp->shmem_signature, (char*)SHMEM_SIGNATURE, SHMEM_SIGNLEN );

  return shmem_ptr;
}

int shmem_free( void* shmem_ptr )
{
  int flags, ret;
  SHMEM_HEADER* shmem_hdrp;

  shmem_hdrp = (SHMEM_HEADER*)( (char*)shmem_ptr-SHMEM_HDRLEN );
  if( memcm( shmem_hdrp->shmem_signature, SHMEM_SIGNATURE, SHMEM_SIGNLEN ) )
     return -1; // Not shared mem.

  flags=set_disable();
  if( shmem_hdrp->shmem_status&SHMEM_FREE )
  {
    memse((char*) shmem_hdrp->shmem_signature, 0, SHMEM_SIGNLEN );
    dealloc((void*)shmem_hdrp);
    ret = 0;
  }
  else
  {
    shmem_hdrp->shmem_status|=SHMEM_FRREQ;
    ret = 1;
  }
  set_enable(flags);

  return ret;
}

int shmem_rdlock( void* shmem_ptr, int just_try )
{
  int flags, ret;
  SHMEM_HEADER* shmem_hdrp;
  LINK suspptr;

  shmem_hdrp = (SHMEM_HEADER*)( (char*)shmem_ptr-SHMEM_HDRLEN );
  if( memcm( shmem_hdrp->shmem_signature, SHMEM_SIGNATURE, SHMEM_SIGNLEN ) )
     return -1; // Not shared mem.

  if( shmem_hdrp->shmem_status&SHMEM_FRREQ )
     return -2; // Mem should be released.

  flags=set_disable();

  if( shmem_hdrp->shmem_status&SHMEM_FREE )
  {
    shmem_hdrp->shmem_status&=~SHMEM_FREE; // Not free anymoer.
    shmem_hdrp->shmem_status|=SHMEM_READ;  // REading operation.
    ret = 0; 
  }
  else if( shmem_hdrp->shmem_status&SHMEM_READ ) // If someone is already reading.
  {
    if( shmem_hdrp->S_QH // Check queue.
        && ( (shmem_hdrp->shmem_status&SHMEM_FIFO) // Check FIFO.
          || (shmem_hdrp->S_QH->I_PRIORITY<=running->I_PRIORITY)) // Check priority.
      )
        ret = 1; // Block task.
    else // There are no tasks waiting or current task has the highest priority.

      ret = 0; 
  }
  else if( shmem_hdrp->shmem_status&SHMEM_WRITE ) // Someone is writing.
    ret = 1; // Block task.
  else
    ret = -3; // Something is wrong.

  if( ret==0 )
  {
    shmem_hdrp->readin_counter++;
  }
  else if( ret==1 && !just_try)
  {
    suspptr = running;
    suspptr->next_sem_susp = NULL;

    if (shmem_hdrp->S_QH == NULL)
    {
      shmem_hdrp->S_QH = shmem_hdrp->S_QT = suspptr;
    }
    else
    {
      if( shmem_hdrp->shmem_status&SHMEM_FIFO )
      {
          shmem_hdrp->S_QT->next_sem_susp = suspptr;
          shmem_hdrp->S_QT = suspptr;
      }
      else
        semqueue_by_priority( &shmem_hdrp->S_QH, &shmem_hdrp->S_QT, suspptr);
    }

      running->I_UZROK  = SUSPEND_ON_SHMEMRD;
      running->I_STANJE = TASK_WAIT_SUSP;     // Waiting on suspension. 

      set_enable(flags);
      while (running->I_STANJE == TASK_WAIT_SUSP)
        if (disp_mode&ROUND_ROBIN)
          calldisp ();
  }

  set_enable(flags);

  return ret;
}

int shmem_wrlock( void* shmem_ptr, int just_try )
{
  int flags, ret;
  SHMEM_HEADER* shmem_hdrp;
  LINK suspptr;

  shmem_hdrp = (SHMEM_HEADER*)( (char*)shmem_ptr-SHMEM_HDRLEN );
  if( memcm( shmem_hdrp->shmem_signature, SHMEM_SIGNATURE, SHMEM_SIGNLEN ) )
     return -1; // Not shared mem.

  if( shmem_hdrp->shmem_status&SHMEM_FRREQ )
     return -2;// Mem should be released.

  flags=set_disable();

  if( shmem_hdrp->shmem_status&SHMEM_FREE )
  {
    shmem_hdrp->shmem_status&=~SHMEM_FREE; // Not free anymoer.
    shmem_hdrp->shmem_status|=SHMEM_WRITE;  // Writing operation.
    ret = 0; 
  }
  else if( shmem_hdrp->shmem_status&SHMEM_READ ||
           shmem_hdrp->shmem_status&SHMEM_WRITE
         ) // If someone is already writing or reading.
    ret = 1; 
  else
    ret = -3; // Something is wrong.

  if( ret==1 && !just_try)
  {
    suspptr = running;
    suspptr->next_sem_susp = NULL;

    if (shmem_hdrp->S_QH == NULL)
    {
      shmem_hdrp->S_QH = shmem_hdrp->S_QT = suspptr;
    }
    else
    {
      if( shmem_hdrp->shmem_status&SHMEM_FIFO )
      {
          shmem_hdrp->S_QT->next_sem_susp = suspptr;
          shmem_hdrp->S_QT = suspptr;
      }
      else
        semqueue_by_priority( &shmem_hdrp->S_QH, &shmem_hdrp->S_QT, suspptr);
    }

      running->I_UZROK  = SUSPEND_ON_SHMEMWR;
      running->I_STANJE = TASK_WAIT_SUSP;     // Waiting on suspension. 

      set_enable(flags);
      while (running->I_STANJE == TASK_WAIT_SUSP)
        if (disp_mode&ROUND_ROBIN)
          calldisp ();
  }

  set_enable(flags);

  return ret;
}


int shmem_unlock( void* shmem_ptr )
{
  int flags, ret;
  SHMEM_HEADER* shmem_hdrp;
  LINK suspptr;

  shmem_hdrp = (SHMEM_HEADER*)( (char*)shmem_ptr-SHMEM_HDRLEN );
  if( memcm( shmem_hdrp->shmem_signature, SHMEM_SIGNATURE, SHMEM_SIGNLEN ) )
     return -1; // Not shared mem.

  flags=set_disable();

  if( shmem_hdrp->shmem_status&SHMEM_READ ) // If operation is reading.
  {
    if( --shmem_hdrp->readin_counter>0 )
      ret=0;
    else
    {
      shmem_hdrp->shmem_status&=~SHMEM_READ; // Finish reading.
      if( shmem_hdrp->S_QH )
        ret = 1; 
      else
      {
        shmem_hdrp->shmem_status|=SHMEM_FREE;  // Shared mem is free now.
        ret = 0;
      }
    }
  }
  else if( shmem_hdrp->shmem_status&SHMEM_WRITE ) // If operation is writing.
  {
    shmem_hdrp->shmem_status&=~SHMEM_WRITE; // Finish writing.
    if( shmem_hdrp->S_QH )
      ret = 1; 
    else
    {
      shmem_hdrp->shmem_status|=SHMEM_FREE;  // SHared mem is free now.
      ret = 0;
    }
  }
  else
    ret = -3; // Something is wrong.

  if( ret==1 ) // Unblock task.
  {
    do
    {
      suspptr = shmem_hdrp->S_QH;

      if ( suspptr->I_STANJE == TASK_SUSPENDED)
      {
        suspptr->I_STANJE = TASK_READY;
        queue ( suspptr );
      }
      else if ( suspptr->I_STANJE == TASK_WAIT_SUSP)
        suspptr->I_STANJE = TASK_READY;

      if( shmem_hdrp->S_QH==shmem_hdrp->S_QT )
        shmem_hdrp->S_QH = shmem_hdrp->S_QT = NULL;
      else
        shmem_hdrp->S_QH = shmem_hdrp->S_QH->next_sem_susp;

         if( suspptr->I_UZROK==SUSPEND_ON_SHMEMRD )
         {
            shmem_hdrp->readin_counter++;
            shmem_hdrp->shmem_status|=SHMEM_READ;
         }
         else
            shmem_hdrp->shmem_status|=SHMEM_WRITE;
    } while( suspptr->I_UZROK==SUSPEND_ON_SHMEMRD );

    if (disp_mode&ROUND_ROBIN)
    {
      set_enable (flags);
      dispatch();
      disable ();
    }
  }

  set_enable(flags);

  return ret;
}

/////////////////////////////////////////////////////////////////////

/**
 * Extern main function
 *
 * This function is the first user-created task defined in main.c
 *
 * @param params parameters
 */
extern void main(void * params);

/**
 * Logger
 *
 * @param params parameters
 */
static void logger(void * params){
  print_str("\n\rAll tasks have finished!"); 
  while(1){  
    RPI_WaitMicroSeconds(1000000);
  }
}

/**
 * Kernel main function.
 *
 * This functio initializes the MT system by doing the following:
 *  1. Initialize Mini UART,
 *  2. Initialize interrupts - enable only timer interrupts,
 *  3. Initialize timer interrupt interval by setting timer counter,
 *  4. Initialize timer mode,
 *  5. Initialize heap,
 *  6. Initialize mt,
 *  7. Enable interrupts, create and start main task.
 * 
 * @param r0 register 0 
 * @param r1 register 1
 * @param atags register 2
 */
void kernel_main( uint32_t r0, uint32_t r1, uint32_t atags )
{
    (void) r0;
    (void) r1;
    (void) atags;

    disable();

    RPI_AuxMiniUartInit(115200, 8);
    print_str("Initialization complete\n\r");
	
	RPI_SetGpioPinFunction( RPI_GPIO47, FS_OUTPUT );
    RPI_SetGpioHi( RPI_GPIO47 );
        
    /* Enable the timer interrupt IRQ */
    RPI_GetIrqController()->Enable_Basic_IRQs = RPI_BASIC_ARM_TIMER_IRQ;

    /* Setup the system timer interrupt */
    /* Timer frequency = Clk/256 * 0x58 */
    RPI_GetArmTimer()->Load = 0x58; // around 1ms ~ 976 ticks per second
  
    
    /* Setup the ARM Timer */
    RPI_GetArmTimer()->Control =
            RPI_ARMTIMER_CTRL_23BIT |
            RPI_ARMTIMER_CTRL_ENABLE |
            RPI_ARMTIMER_CTRL_INT_ENABLE |
            RPI_ARMTIMER_CTRL_PRESCALE_256;


    mem_init();    

    initmt(TIME_AND_ROUND,1);
    
    enable();
    
	// Start main task with default priority
    sptsk(create_task(main, STACK_SIZE, NULL, 0), PRDEFAULT);
   
    // Start logging task
    sptsk(create_task(logger, STACK_SIZE, NULL, 0), PRMAX - 1);
    
    /* Never exit as there is no OS to exit to! */
    _halt();
}

