#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define NUM_SEARCHERS	10
#define NUM_INSERTERS	10
#define NUM_DELETERS	10

#define MAX_DATA	32

typedef unsigned int ticket_t;

struct shared_data {
	int data[MAX_DATA];

	pthread_cond_t cond_zero;	/* Did one of the thread counts hit 0? */
	pthread_mutex_t lock_data;	/* Lock the extra data below (not data array) */

	ticket_t now_servicing;		/* Which ticket are we servicing? */
	ticket_t ticket_num;		/* Next ticket */

	int num_data;			/* Number of data in data[] */
	int num_searchers;		/* Number of searchers searching */
	int num_inserters;		/* Number of inserters inserting */
	int num_deleters;		/* Number of deleters deleting */
};

void shared_data_init(struct shared_data *sd)
{
	pthread_cond_init(&sd->cond_zero, NULL);
	pthread_mutex_init(&sd->lock_data, NULL);
	
	sd->now_servicing = 0;
	sd->ticket_num = 0;

	sd->num_data = 0;
	sd->num_searchers = 0;
	sd->num_inserters = 0;
	sd->num_deleters = 0;
}

void *searcher(void *arg)
{
	struct shared_data *sd = (struct shared_data *)arg;
	ticket_t ticket;

	
	for (;;) {
		pthread_mutex_lock(&sd->lock_data);
		ticket = sd->ticket_num++;
		printf("Searcher taking ticket #%d\n", ticket);
		
		while (sd->num_deleters != 0 || ticket != sd->now_servicing) {
			pthread_cond_wait(&sd->cond_zero, &sd->lock_data);
		}
		
		sd->num_searchers++;
		sd->now_servicing++;
		pthread_mutex_unlock(&sd->lock_data);

		// SEARCH HERE
		printf("Searching\n");
		
		pthread_mutex_lock(&sd->lock_data);
		sd->num_searchers--;
		if (sd->num_searchers == 0) {
			pthread_cond_broadcast(&sd->cond_zero);
		}	
		pthread_mutex_unlock(&sd->lock_data);
	}
}

void *inserter(void *arg)
{
	struct shared_data *sd = (struct shared_data *)arg;
	ticket_t ticket;

	for (;;) {
		pthread_mutex_lock(&sd->lock_data);
		ticket = sd->ticket_num++;
		printf("Inserter taking ticket #%d\n", ticket);

		while (sd->num_deleters != 0 || sd->num_inserters != 0 || sd->now_servicing != ticket) {
			pthread_cond_wait(&sd->cond_zero, &sd->lock_data);
		}

		sd->num_inserters++;
		sd->now_servicing++;
		pthread_mutex_unlock(&sd->lock_data);

		// INSERT HERE
		printf("Inserting\n");
		
		pthread_mutex_lock(&sd->lock_data);
		sd->num_inserters--;
		if (sd->num_inserters == 0) {
			pthread_cond_broadcast(&sd->cond_zero);
		}
		pthread_mutex_unlock(&sd->lock_data);
	}
}

void *deleter(void *arg)
{
	struct shared_data *sd = (struct shared_data *)arg;
	ticket_t ticket;

	for (;;) {
		pthread_mutex_lock(&sd->lock_data);
		ticket = sd->ticket_num++;
		printf("Deleter taking ticket #%d\n", ticket);

		while (sd->num_deleters != 0 || sd->num_inserters != 0 || sd->num_deleters != 0 || sd->now_servicing != ticket) {
			pthread_cond_wait(&sd->cond_zero, &sd->lock_data);
		}

		sd->num_deleters++;
		sd->now_servicing++;
		pthread_mutex_unlock(&sd->lock_data);

		// DELETE HERE
		printf("Deleting\n");
		
		pthread_mutex_lock(&sd->lock_data);
		sd->num_deleters--;
		if (sd->num_deleters == 0) {
			pthread_cond_broadcast(&sd->cond_zero);
		}
		pthread_mutex_unlock(&sd->lock_data);
	}
}

/* Main */
int main()
{
	struct shared_data sd;
	shared_data_init(&sd);
	int i;

	srand(time(NULL));
	
	pthread_t searchers[NUM_SEARCHERS];
	for(i = 0; i < NUM_SEARCHERS; i++)
		pthread_create(&searchers[i], NULL, searcher, &sd);


	pthread_t inserters[NUM_INSERTERS];
	for(i = 0; i < NUM_INSERTERS; i++)
		pthread_create(&inserters[i], NULL, inserter, &sd);

	pthread_t deleters[NUM_DELETERS];
	for(i = 0; i < NUM_DELETERS; i++)
		pthread_create(&deleters[i], NULL, deleter, &sd);
	
	for(;;) {
		sleep(1);
		pthread_mutex_lock(&sd.lock_data);
		printf("Now servicing ticket #%d. S:%d I:%d D:%d.\n", sd.now_servicing, sd.num_searchers, sd.num_inserters, sd.num_deleters);
		pthread_mutex_unlock(&sd.lock_data);
	}
	pause();
	
	return 0;
}
