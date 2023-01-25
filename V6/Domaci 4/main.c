#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_LEN 80

static char states[4] = "0000";
static pthread_mutex_t led_mutex;
int file_desc;

void* led_thread(void *param) {
    int p = *(int*) param;
    char tmp[BUF_LEN], output[BUF_LEN];
    while (1) {
        pthread_mutex_lock(&led_mutex);
        
        // Open file
        file_desc = open("/dev/gpio_driver", O_RDWR);
        if (file_desc < 0) {
            printf("Error, file not opened\n");
            exit(1);
        }

        read(file_desc, tmp, BUF_LEN);

        // Check if state of switch changed
        if (states[p] != tmp[p]) {
            states[p] = tmp[p];
            sprintf(output, "LED %d %d", p, states[p]);
            write(file_desc, output, BUF_LEN);
        }

        // Close file
        close(file_desc);

        // Unlock mutex and let other thread continue
        pthread_mutex_unlock(&led_mutex);
        usleep(100);
    }
}

int main()
{
    pthread_t hLED0;
    pthread_t hLED1;
    pthread_t hLED2;
    pthread_t hLED3;
	
    pthread_mutex_init(&led_mutex,NULL);

    int led0 = 0;
    int led1 = 1;
    int led2 = 2;
    int led3 = 3;
   
    pthread_create(&hLED0, NULL , led_thread, (void*)&led0);
    pthread_create(&hLED1, NULL , led_thread, (void*)&led1);
    pthread_create(&hLED2, NULL , led_thread, (void*)&led2);
    pthread_create(&hLED3, NULL , led_thread, (void*)&led3);

    pthread_join(hLED0, NULL);
    pthread_join(hLED1, NULL);
    pthread_join(hLED2, NULL);
    pthread_join(hLED3, NULL);

    pthread_mutex_destroy(&led_mutex);
    
    return 0;
}
