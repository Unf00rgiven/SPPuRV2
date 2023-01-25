
#include "kernel.h"
#include "types-mt.h"
#include "rpi-aux.h"
#include "rpi-systimer.h"
#include "stdlib.h"
#include "types-mt.h"
#include "tasks.h"

#define RING_SIZE  (16)
#define SLEEPING_TIME           800000
#define LOWER_TO_UPPER_CASE     0x20

void simulate_processing();

/*  Kruzni bafer - FIFO. */
struct RingBuffer{
  uint32_t tail;
  uint32_t head;
  char  data[RING_SIZE];
};

/* Operacije za rad sa kruznim baferom. */
char ringBufGetChar(struct RingBuffer * apBuffer){
    int index;
    index = apBuffer->head;
    apBuffer->head = (apBuffer->head + 1) % RING_SIZE;
    return apBuffer->data[index];
}

void ringBufPutChar(struct RingBuffer * apBuffer, const char c){
    apBuffer->data[apBuffer->tail] = c;
    apBuffer->tail = (apBuffer->tail + 1) % RING_SIZE;
}

/* Globalne promenljive. */

/* Kruzni bafer. */

static struct RingBuffer inputBuffer;
static struct RingBuffer outputBuffer;

static SEM semInEmpty;
static SEM semInFull;
static SEM semOutEmpty;
static SEM semOutFull;
static SEM semFinishSignal;

static MUTEX inputBufferAccess;
static MUTEX outputBufferAccess;

/* Funkcija procesa proizvodjaca */
void producer(void * param){
  char c;

  while(1){
    if(tryrsvsm(&semFinishSignal) == TRUE){
      break;
    }
    
    if(tryrsvsm(&semInEmpty) == TRUE){

      /* Funkcija za unos karaktera sa tastature. */
      c = getch();
	  
      /* Ukoliko je unet karakter q signalizira se
      svim procesima zavrsetak rada. */
      if(c == 'q'){
        /* Zaustavljanje procesa; Semafor se oslobadja 3 puta
        da bi signaliziralo svim procesima.*/
        rlsm(&semFinishSignal);
        rlsm(&semFinishSignal);
        rlsm(&semFinishSignal);
      }

      /* Pristup kruznom baferu. */
      rsvmtx(&inputBufferAccess);
      ringBufPutChar(&inputBuffer, c);
      rlsmtx(&inputBufferAccess);

      rlsm(&semInFull);      
    }
  }
}

/* Funkcija procesa potrosaca */
void consumer(void * param){
  char c;

  print_str("Znakovi preuzeti iz kruznog bafera:\n\r");

  while(1){
    if(tryrsvsm(&semFinishSignal) == TRUE){
      break;
    }

    if(tryrsvsm(&semOutFull) == TRUE){
      /* Pristup kruznom baferu. */
      rsvmtx(&outputBufferAccess);
      c = ringBufGetChar(&outputBuffer);
      rlsmtx(&outputBufferAccess);

      /* Ispis na konzolu. */
      print_char(c);

      rlsm(&semOutEmpty);
    }
  }
}

/* Funkcija procesa radnika */
void converter(void * params){
  char c;

  while(1){
    if(tryrsvsm(&semFinishSignal) == TRUE){
      break;
    }

    if(tryrsvsm(&semInFull) == TRUE){
      rsvmtx(&inputBufferAccess);
      c = ringBufGetChar(&inputBuffer);
      rlsmtx(&inputBufferAccess);
	  	
	  if(c >= 'a' && c <= 'z'){
		c -= LOWER_TO_UPPER_CASE;
	  }
	  
	  /* Simulacija trajanja posla */
	  simulate_processing();
	  
      while(1){
        if(tryrsvsm(&semOutEmpty) == TRUE){
          rsvmtx(&outputBufferAccess);
          ringBufPutChar(&outputBuffer, c);
          rlsmtx(&outputBufferAccess);
          rlsm(&semOutFull);
          break;
        }
      }
      rlsm(&semInEmpty);
    }
  }
}

/* Glavni proces koji formira preostale procese (proizvodjac, potrosac i radnik) i ceka njihovo gasenje. */
void main(void * params){
	
  /* Identifikatori procesa. */
  int hProducer;
  int hConverter;
  int hConsumer;
  
  /* Pauziraj izvrsavanje dok korisnik ne posalje naredbu */
  print_str("Press Enter to start the execution!\n\r");
  getch();

  /* Formiranje semEmpty, semFull i semFinishSignal semafora. */
  dfsm(&semInEmpty, RING_SIZE);
  dfsm(&semInFull, 0);
  dfsm(&semOutEmpty, RING_SIZE);
  dfsm(&semOutFull, 0);
  dfsm(&semFinishSignal, 0);
  
  /* Inicijalizacija objekta iskljucivog pristupa. */
  dfmtx(&inputBufferAccess);
  dfmtx(&outputBufferAccess);

  /* Formiranje procesa: proizodjac, potrosac i radnik */
  hProducer = create_task(producer, STACK_SIZE, NULL, 0);
  hConsumer = create_task(consumer, STACK_SIZE, NULL, 0);
  hConverter = create_task(converter, STACK_SIZE, NULL, 0);

  sptsk(hProducer, PRDEFAULT);
  sptsk(hConsumer, PRDEFAULT);
  sptsk(hConverter, PRDEFAULT);
  
  levelchange(PRDEFAULT+1);
  
  print_str("Oslobadjanje memorije i uklanjanje zadataka\n\r");
  
  kill_task(hProducer);
  kill_task(hConsumer);
  kill_task(hConverter);
  
}

void simulate_processing()
{
	RPI_WaitMicroSeconds(SLEEPING_TIME);
}