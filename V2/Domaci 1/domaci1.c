#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

static pthread_mutex_t mutex;                   //mutex koji koristim
static int id;                                  //promenljiva koja signalizira koja nit je na redu
static int q;                                   //broj koliko puta se ispisala sekvenca '123'

/* Telo niti. */
void* print (void *pParam){
    char c = *(char*)pParam;

    while(1){                                   //while se vrti sve dok ne dobijem sto sekvenci
        pthread_mutex_lock(&mutex);

        if(id %3 == 0){                         //treba da se izvrsi 1. nit
            if(c == '1'){                       //u pitanju jeste prva nit
                printf("%c", c);
                fflush(stdout);
                id++;                           //povecava se na nesto sto ce dati ostatak pri deljenju 1  
            }
            pthread_mutex_unlock(&mutex);       //oslobadjam mutex, i ako je ispisana 1. nit i ako nije
            continue;
        }else if(id %3 == 1){
            if(c == '2'){    
                printf("%c", c);
                fflush(stdout);
                id++;                           //povecava se na nesto sto ce dati ostatak pri deljenju 2
            } 
            pthread_mutex_unlock(&mutex);
            continue;   
        }else{                                  //if(id %3 == 2)                
            if(c == '3'){
                printf("%c", c);
                fflush(stdout);
                id++;                           //povecava se na nesto sto ce dati ostatak pri deljenju 0
                q++;                            //dosao sam do kraja jedne sekvence
                if(q == 100){                   //ako je broj sekvenci 100, kraj funkcije
                    return 0;
                }
            }
            pthread_mutex_unlock(&mutex);
            continue; 
        }
    }
}

int main (void)
{                   
    pthread_t hPrint1;
    pthread_t hPrint2;
    pthread_t hPrint3;

    /* Argumenti niti. */
    char c1 = '1';
    char c2 = '2';
    char c3 = '3';

    pthread_mutex_init(&mutex, NULL);                           //inicijalizacija mutexa

    id = 0;                                                     //inicijalizacija statickih promenljivih
    q = 0;

    pthread_create(&hPrint1, NULL, print, &c1);
    pthread_create(&hPrint2, NULL, print, &c2);
    pthread_create(&hPrint3, NULL, print, &c3);

    getchar();                                                  //ima funkciju join()

    pthread_mutex_destroy(&mutex);                              //unistenje mutexa

    return 0;
}
