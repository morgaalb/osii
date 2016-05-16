#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#define NUM_WORKERS 5
#define MIN_WORKTIME 3
#define MAX_WORKTIME 6
#define MIN_WAITTIME 3
#define MAX_WAITTIME 6

struct shared_data {
	pthread_mutex_t data_lock;
	pthread_cond_t wake_cond;
	int users;
	int spindown;
};

void shared_data_init(struct shared_data *data)
{
	pthread_mutex_init(&data->data_lock, NULL);
	pthread_cond_init(&data->wake_cond, NULL);
	data->users = 0;
	data->spindown = 0;
}

void *worker(void *arg)
{
	struct shared_data *data = (struct shared_data *)arg;
	int r;

	for(;;) {
		// Register this thread as using the resource
		pthread_mutex_lock(&data->data_lock);	
		while(data->users >= 3 || data->spindown) {
			pthread_cond_wait(&data->wake_cond, &data->data_lock);
		}
		data->users++;
		r = random() % (MAX_WORKTIME - MIN_WORKTIME + 1) + MIN_WORKTIME;
		printf("Timestamp %lu: thread %lu starting work for %ds. Number of threads: %d\n", time(NULL), pthread_self(), r, data->users);
		if (data->users >= 3) {
			data->spindown = 1;
			printf("Three users detected. Resource locked.\n");
		}
		
		pthread_mutex_unlock(&data->data_lock);

		// Simulate work
		
		sleep(r);

		// Unregister and clean up the locks
		pthread_mutex_lock(&data->data_lock);
		data->users--;		
		r = random() % (MAX_WAITTIME - MIN_WAITTIME + 1) + MIN_WAITTIME;
		printf("Timestamp %lu: tread %lu ending work. Sleep for %ds. Number of threads: %d\n", time(NULL), pthread_self(), r, data->users);
		if (data->users <= 0) {
			data->spindown = 0;
			printf("Zero users detected. Resource unlocked.\n");
		}
		pthread_mutex_unlock(&data->data_lock);
		pthread_cond_broadcast(&data->wake_cond);

		sleep(r);
	}
}


void main()
{
	printf("Starting with %d worker threads.\n", NUM_WORKERS);
	struct shared_data data;
	int i;

	shared_data_init(&data);	
	srand(time(NULL));
	
	pthread_t workers[NUM_WORKERS];
	for(i = 0; i < NUM_WORKERS; i++)
		pthread_create(&workers[i], NULL, worker, &data);

	pause();
}
