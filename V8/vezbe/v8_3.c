
#include "tasks.h"
#include "kernel.h"
#include "types-mt.h"
#include "rpi-aux.h"
#include "stdlib.h"

static SEM semaphore;

static int counter;

void counter_thread(void * params){
  while(1){
    rsvsm(&semaphore);
    
    counter++;
    print_str("counter = ");
    print_num(counter);
    print_str("\n\r");
  }
}

void main(void * params){

  int hThread;

  /* Pauziraj izvrsavanje dok korisnik ne posalje naredbu */
  print_str("Press Enter to start the execution!\n\r");
  getch();

  /* Inicijaliizacija semafora */
  dfsm(&semaphore, 0);

  /* Pravljenje procesa */
  hThread = create_task(counter_thread, STACK_SIZE, NULL, 0);

  /* Pokretanje procesa */
  sptsk(hThread, PRDEFAULT);
  
  while(1){
    if(getch() == 'q'){
      break;
    }
    rlsm(&semaphore);
  }
  
}


