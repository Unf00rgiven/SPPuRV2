
#include "tasks.h"
#include "kernel.h"
#include "types-mt.h"
#include "rpi-aux.h"
#include "stdlib.h"

void print1(void * params){
  int i;
  
  for(i = 0; i < 100; i++){
    
    print_str("Zvezvda");
    print_str(" je");
    print_str(" prvak");
    print_str(" jsveta\n\r");
   
  }
}

void print2(void * params){
  int i;
  
  for(i = 0; i < 100; i++){
    
    print_str("Priprema");
    print_str(" vezbe");
    print_str(" iz");
    print_str(" sistemske");
    print_str(" programske");
    print_str(" podrske\n\r");
    
  }
}

void print3(void * params){
  int i;
  
  for(i = 0; i < 100; i++){
    
    print_str("Danas");
    print_str(" je");
    print_str(" lep");
    print_str(" i");
    print_str(" suncan");
    print_str(" dan\n\r");
    
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


