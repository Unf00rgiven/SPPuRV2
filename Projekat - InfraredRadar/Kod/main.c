#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define BUF_LEN 80
#define SLEEPING_TIME 100000

static pthread_mutex_t sensorMutex;
int file_desc;

void* sensor_thread(void* param)
{
	int sensor_num= *(int*)param;

	char read_buf[BUF_LEN];

	int val_prev = 0;
	int val_next = 0;

	while(1)
	{
		pthread_mutex_lock(&sensorMutex);
		file_desc = open("/dev/gpio_driver", O_RDWR);

		if(file_desc < 0)
		{
			pthread_mutex_unlock(&sensorMutex);
			printf("Not opened");
			break;
		}

		if(read(file_desc, read_buf, BUF_LEN) <= 0)
		{
			close(file_desc);
			pthread_mutex_unlock(&sensorMutex);
			printf("Read failed, %d" , sensor_num);
			continue;
		}

		val_next = read_buf[sensor_num] - '0';

		if(val_next != val_prev)
		{
			if(sensor_num == 0 && val_next == 1)
			    printf("Nema objekata ispred senzora %d.\n" , sensor_num);
			if(sensor_num == 2 && val_next == 1)
			    printf("Nema objekata ispred senzora %d.\n" , sensor_num);
			if(sensor_num == 0 && val_next == 0)
			    printf("Objekat se nalazi ispred senzora %d.\n" , sensor_num);
			if(sensor_num == 2 && val_next == 0)
			    printf("Objekat se nalazi ispred senzora %d.\n" , sensor_num);
			if(sensor_num == 1 && val_next == 1)
			    printf("Objekat se ne nalazi izmedju senzora tj. ispred senzora %d.\n", sensor_num);
			if(sensor_num == 1 && val_next == 0)
			    printf("Objekat se nalazi izmedju senzora tj. ispred senzora %d.\n", sensor_num);
			if(sensor_num == 3 && val_next == 1)
			    printf("Objekat se ne nalazi izmedju senzora tj. ispred senzora %d.\n", sensor_num);
			if(sensor_num == 3 && val_next == 0)
			    printf("Objekat se nalazi izmedju senzora tj. ispred senzora %d.\n", sensor_num);

		}

		close(file_desc);
		pthread_mutex_unlock(&sensorMutex);
		usleep(SLEEPING_TIME);
		val_prev = val_next;
	}

	return 0;

}

int main()
{
    pthread_t hSENSOR0;
    pthread_t hSENSOR1;
    pthread_t hSENSOR2;
    pthread_t hSENSOR3;

    pthread_mutex_init(&sensorMutex,NULL);

    int sensor0 = 0;
    int sensor1 = 1;
    int sensor2 = 2;
    int sensor3 = 3;

    pthread_create(&hSENSOR0, NULL , sensor_thread, (void*)&sensor0);
    pthread_create(&hSENSOR1, NULL , sensor_thread, (void*)&sensor1);
    pthread_create(&hSENSOR2, NULL , sensor_thread, (void*)&sensor2);
    pthread_create(&hSENSOR3, NULL , sensor_thread, (void*)&sensor3);

    pthread_join(hSENSOR0, NULL);
    pthread_join(hSENSOR1, NULL);
    pthread_join(hSENSOR2, NULL);
    pthread_join(hSENSOR3, NULL);

    pthread_mutex_destroy(&sensorMutex);

    return 0;
}
