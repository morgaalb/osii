#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define NUM_SEARCHERS	10
#define NUM_INSERTERS	3
#define NUM_DELETERS	3

#define MAX_DATA	1000

/* Turn this on to have the thread sleep after doing its operation.
 * Useful for checking that the behavior is correct.
 */
#define ARTIFICIAL_SLEEP	1

typedef unsigned int ticket_t;

struct shared_data {
	int data[MAX_DATA];

	pthread_cond_t cond_zero;	/* Did one of the thread counts hit 0? */
	pthread_mutex_t lock_data;	/* Lock the extra data below (not data array) */

	ticket_t now_servicing;		/* Which ticket are we servicing? */
	ticket_t ticket_num;		/* Next ticket */

	int client_num;			/* For indexing clients */

	int num_items;			/* Number of data in data[] */
	int num_searchers;		/* Number of searchers searching */
	int num_inserters;		/* Number of inserters inserting */
	int num_deleters;		/* Number of deleters deleting */
};

/* Initializes the shared data struct */
void shared_data_init(struct shared_data *sd)
{
	pthread_cond_init(&sd->cond_zero, NULL);
	pthread_mutex_init(&sd->lock_data, NULL);
	
	sd->now_servicing = 0;
	sd->ticket_num = 0;

	sd->client_num = 0;
	sd->num_items = 0;
	sd->num_searchers = 0;
	sd->num_inserters = 0;
	sd->num_deleters = 0;
}

/* Searcher thread */
void *searcher(void *arg)
{
	struct shared_data *sd = (struct shared_data *)arg;
	ticket_t ticket;
	int i;
	int value;

	int my_num;
	pthread_mutex_lock(&sd->lock_data);
	my_num = sd->client_num++;
	pthread_mutex_unlock(&sd->lock_data);

	for (;;) {
		pthread_mutex_lock(&sd->lock_data);
		ticket = sd->ticket_num++;
		printf("Searcher %d takes ticket #%d\n", my_num, ticket);
		
		while (sd->num_deleters != 0 || ticket != sd->now_servicing) {
			pthread_cond_wait(&sd->cond_zero, &sd->lock_data);
		}

		printf("Now servicing ticket #%d (Searcher %d). Active threads are S: %d I: %d D: %d. List size is %d\n", sd->now_servicing, my_num,
			sd->num_searchers, sd->num_inserters, sd->num_deleters, sd->num_items);
		
		sd->num_searchers++;
		sd->now_servicing++;
		pthread_cond_broadcast(&sd->cond_zero); // possibly wake up other searchers
		pthread_mutex_unlock(&sd->lock_data);

		// SEARCH HERE
		printf("Searcher %d is searching.\n", my_num);
		value = rand() % 1000;
		for (i = 0; i < sd->num_items; i++) {
			if (sd->data[i] == value) {
				break;
			}
		}

		if (ARTIFICIAL_SLEEP) {
			sleep(rand() % 4);
		}
		
		pthread_mutex_lock(&sd->lock_data);
		sd->num_searchers--;
		if (sd->num_searchers == 0) {
			pthread_cond_broadcast(&sd->cond_zero);
		}	
		pthread_mutex_unlock(&sd->lock_data);
	}
}

/* Inserter thread */
void *inserter(void *arg)
{
	struct shared_data *sd = (struct shared_data *)arg;
	ticket_t ticket;
	int i;
	int place, value;

	int my_num;
	pthread_mutex_lock(&sd->lock_data);
	my_num = sd->client_num++;
	pthread_mutex_unlock(&sd->lock_data);
	
	for (;;) {
		pthread_mutex_lock(&sd->lock_data);
		ticket = sd->ticket_num++;
		printf("Inserter %d takes ticket #%d\n", my_num, ticket);

		while (sd->num_deleters != 0 || sd->num_inserters != 0 || sd->now_servicing != ticket) {
			pthread_cond_wait(&sd->cond_zero, &sd->lock_data);
		}

		printf("Now servicing ticket #%d (Inserter %d). Active threads are S: %d I: %d D: %d. List size is %d\n", sd->now_servicing, my_num,
			sd->num_searchers, sd->num_inserters, sd->num_deleters, sd->num_items);
		
		sd->num_inserters++;
		sd->now_servicing++;
		pthread_mutex_unlock(&sd->lock_data);

		if (sd->num_items < MAX_DATA) {
		
			// INSERT HERE	
			printf("Inserter %d is inserting\n", my_num);
			value = rand() % 1000;
			if (sd->num_items == 0) {
				sd->data[0] = value;
			} else {

				place = rand() % sd->num_items;
				for(i = sd->num_items; i > place; i--) {
					sd->data[i] = sd->data[i - 1];
				}
				sd->data[place] = value;
			}

			sd->num_items++;
		} else {
			printf("Inserter %d found list full.\n", my_num);
		}


		if (ARTIFICIAL_SLEEP) {
			sleep(rand() % 5);
		}
		
		pthread_mutex_lock(&sd->lock_data);
		sd->num_inserters--;
		if (sd->num_inserters == 0) {
			pthread_cond_broadcast(&sd->cond_zero);
		}
		pthread_mutex_unlock(&sd->lock_data);
	}
}

/* Deleter thread */
void *deleter(void *arg)
{
	struct shared_data *sd = (struct shared_data *)arg;
	ticket_t ticket;
	int place, i;

	int my_num;
	pthread_mutex_lock(&sd->lock_data);
	my_num = sd->client_num++;
	pthread_mutex_unlock(&sd->lock_data);

	for (;;) {
		pthread_mutex_lock(&sd->lock_data);
		ticket = sd->ticket_num++;
		printf("Deleter %d takes ticket #%d\n", my_num, ticket);

		while (sd->num_searchers != 0 || sd->num_inserters != 0 || sd->num_deleters != 0 || sd->now_servicing != ticket) {
			pthread_cond_wait(&sd->cond_zero, &sd->lock_data);
		}


		printf("Now servicing ticket #%d (Deleter %d). Active threads are S: %d I: %d D: %d. List size is %d\n", sd->now_servicing, my_num,
			sd->num_searchers, sd->num_inserters, sd->num_deleters, sd->num_items);
		
		sd->num_deleters++;
		sd->now_servicing++;
		pthread_mutex_unlock(&sd->lock_data);

		if (sd->num_items > 0) {		
			// DELETE HERE
			printf("Deleter %d is deleting.\n", my_num);
			place = rand() % sd->num_items;
			for (i = place; i < sd->num_items; i++) {
				sd->data[i] = sd->data[i + 1];
			}
			sd->num_items--;
		} else {
			printf("Deleter %d found list empty.\n", my_num);
		}

		if (ARTIFICIAL_SLEEP) {
			sleep(rand() % 5);
		}
		
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
	
	pause();
	
	return 0;
}
