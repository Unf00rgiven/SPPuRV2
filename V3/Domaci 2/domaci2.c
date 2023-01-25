// Ognjen Stojisavljevic RA 155/2019

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define RING_SIZE  (16)
#define SLEEPING_TIME (400000)

char getch(void);

/*  Kruzni bafer - FIFO. */
struct RingBuffer
{
    unsigned int  tail;
    unsigned int  head;
    unsigned char data[RING_SIZE];
};

/* Operacije za rad sa kruznim baferom. */
char ringBufGetChar (struct RingBuffer *apBuffer)
{
    int index;
    index = apBuffer->head;
    apBuffer->head = (apBuffer->head + 1) % RING_SIZE;
    return apBuffer->data[index];
}

void ringBufPutChar (struct RingBuffer *apBuffer, const char c)
{
    apBuffer->data[apBuffer->tail] = c;
    apBuffer->tail = (apBuffer->tail + 1) % RING_SIZE;
}

/* Globalne promenljive. */

/* Kruzni bafer. */
static struct RingBuffer Ulazring;
static struct RingBuffer Izlazring;

static sem_t semUlazPrazan;
static sem_t semUlazPun;
static sem_t semFinishSignal;
static sem_t semIzlazPrazan;
static sem_t semIzlazPun;

static pthread_mutex_t UlazbufferAccess;
static pthread_mutex_t IzlazbufferAccess;

/* Funkcija programske niti proizvodjaca. */
void* producer (void *param)
{
    char c;

    while (1)
    {
        if (sem_trywait(&semFinishSignal) == 0)
        {
            break;
        }

        if (sem_trywait(&semUlazPrazan) == 0)
        {

            /* Funkcija za unos karaktera sa tastature. */
            c = getch();

            /* Ukoliko je unet karakter q ili Q signalizira se 
               programskim nitima zavrsetak rada. */
            if (c == 'q' || c == 'Q')
            {
                /* Zaustavljanje niti; Semafor se oslobadja 3 puta
                da bi signaliziralo svim nitima.*/
                sem_post(&semFinishSignal);
                sem_post(&semFinishSignal);
                sem_post(&semFinishSignal);
            }

            /* Pristup kruznom baferu. */
            pthread_mutex_lock(&UlazbufferAccess);
            ringBufPutChar(&Ulazring, c);
            pthread_mutex_unlock(&UlazbufferAccess);

            sem_post(&semUlazPun);
        }
    }

    return 0;
}

/* Funkcija programske niti obrade.*/
void* obrada (void *param)
{
    char c;


    while (1)
    {
    	if (sem_trywait(&semFinishSignal) == 0)
        {
            break;
        } 
        
        if ( sem_trywait(&semUlazPun)==0)
		{
			/* Pristup kruznom baferu. */
			pthread_mutex_lock(&UlazbufferAccess);
			c=ringBufGetChar(&Ulazring);
			pthread_mutex_unlock(&UlazbufferAccess);
			
			/* pretvaranje u veliko slovo */
			c-=0x20;
			sem_post(&semUlazPrazan);
	
			sem_wait(&semIzlazPrazan);
		    
		/* Pristup kruznom baferu. */
		pthread_mutex_lock(&IzlazbufferAccess);
		ringBufPutChar(&Izlazring, c);
		pthread_mutex_unlock(&IzlazbufferAccess);
		    
		sem_post(&semIzlazPun);
		}
       
    }

    return 0;
}


/* Funkcija programske niti potrosaca.*/
void* consumer (void *param)
{
    char c;

    printf("Znakovi preuzeti iz kruznog bafera:\n");

    while (1)
    {
        if (sem_trywait(&semFinishSignal) == 0)
        {
            break;
        }

        if (sem_trywait(&semIzlazPun) == 0)
        {
            /* Pristup kruznom baferu. */
            pthread_mutex_lock(&IzlazbufferAccess);
            c = ringBufGetChar(&Izlazring);
            pthread_mutex_unlock(&IzlazbufferAccess);

            /* Ispis na konzolu. */
            printf("%c", c);
            fflush(stdout);

            /* Cekanje da bi se ilustrovalo trajanje obrade. */
            usleep(SLEEPING_TIME);

            sem_post(&semIzlazPrazan);
        }
    }

    return 0;
}


/* Glavna programska nit koja formira dve programske (proizvodjac i potrosac) niti i ceka njihovo gasenje. */
int main (void)
{
    /* Identifikatori niti. */
    pthread_t hProducer;
    pthread_t hObrada;
    pthread_t hConsumer;

    /* Formiranje ulaznih i izlaznih semEmpty, semFull i semFinishSignal semafora. */
    sem_init(&semUlazPrazan, 0, RING_SIZE);
    sem_init(&semUlazPun, 0, 0);
    sem_init(&semIzlazPrazan, 0, RING_SIZE);
    sem_init(&semIzlazPun, 0, 0);
    sem_init(&semFinishSignal, 0, 0);

    /* Inicijalizacija objekta iskljucivog pristupa. */
    pthread_mutex_init(&UlazbufferAccess, NULL);
    pthread_mutex_init(&IzlazbufferAccess, NULL);

    /* Formiranje programskih niti: proizodjac i potrosac i obrada. */
    pthread_create(&hProducer, NULL, producer, 0);
    pthread_create(&hObrada, NULL, obrada, 0);
    pthread_create(&hConsumer, NULL, consumer, 0);

    /* Cekanje na zavrsetak formiranih niti. */
    pthread_join(hConsumer, NULL);
    pthread_join(hObrada, NULL);
    pthread_join(hProducer, NULL);

    /* Oslobadjanje resursa. */
    sem_destroy(&semUlazPrazan);
    sem_destroy(&semUlazPun);
    sem_destroy(&semIzlazPrazan);
    sem_destroy(&semIzlazPun);
    sem_destroy(&semFinishSignal);
    pthread_mutex_destroy(&UlazbufferAccess);
    pthread_mutex_destroy(&IzlazbufferAccess);

    printf("\n");

    return 0;
}
