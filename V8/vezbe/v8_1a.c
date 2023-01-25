
#include "tasks.h"
#include "kernel.h"
#include "stdlib.h"
#include "types-mt.h"
#include "rpi-aux.h"

/* Proces 1 - ispisuje broj 1 */
void print1(void * params){
  int i;
  
  for(i = 0; i < 1000; i++){
    print_str("1");
  }
}

/* Proces 2 - ispisuje broj 2 */
void print2(void * params){
  int i;
  
  for(i = 0; i < 1000; i++){
    print_str("2");
  }
}

/* Proces 3 - ispisuje broj 3 */
void print3(void * params){
  int i;
  
  for(i = 0; i < 1000; i++){
    print_str("3");
  }
}

void main(void * params){

  int hPrint1, hPrint2, hPrint3;

  /* Pauziraj izvrsavanje dok korisnik ne posalje naredbu */
  print_str("Press Enter to start the execution!\n\r");
  getch();
 
  /* Pravljenje zadataka */
  hPrint1 = create_task(print1, STACK_SIZE, NULL, 0);
  hPrint2 = create_task(print2, STACK_SIZE, NULL, 0);
  hPrint3 = create_task(print3, STACK_SIZE, NULL, 0);

  /* Pokretanje zadataka - dodavanje u listu pripravnih */
  sptsk(hPrint1, PRDEFAULT);
  sptsk(hPrint2, PRDEFAULT);
  sptsk(hPrint3, PRDEFAULT);
}


