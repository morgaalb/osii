#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define str_helper(s) #s
#define str(s) str_helper(s)

#define NUM_PHILOSOPHERS 	5

/* Controls for the size of the status table */
#define TABLE_WIDTH 		37
#define TABLE_COL1_WIDTH 	15
#define TABLE_COL2_WIDTH 	15

#define THINKING		0
#define EATING 			1
#define WAITING 		2
#define TANTRUM 		3
#define PUT_FORKS		4

/* Define this to make philosophers act 1000x faster. For debugging / fun */
#define FAST_MODE

/* Time before a philosopher throws a tantrum */
#define PATIENCE  		60

/* There must be at least as many names as philosophers! */
const char* names[] = {"Kant", "Heidegger", "Hume", "Schopenhauer", "Hegel"};
/* Human readable state names */
const char* state_str[] = {"Thinking", "Eating", "Waiting", "Tantrum", "Putting forks"};
/* public state information */
int forks[NUM_PHILOSOPHERS];
int states[NUM_PHILOSOPHERS];


/* Threads should lock this mutex before r/w critical data */
pthread_mutex_t mutex_data = PTHREAD_MUTEX_INITIALIZER;
/* Signal that a fork has been set down and may be available */
pthread_cond_t cond_forkdown = PTHREAD_COND_INITIALIZER;

/* Prints a line of hypens */
void print_state_table_line()
{
	int i;
	for(i = 0; i < TABLE_WIDTH; i++) {
		printf("-");
	}
	printf("\n");
}

/* Prints the state. Assumes the data is already locked. */
void print_state_no_lock()
{
	int i;

	for(i = 0; i < 5; i++) {
		printf(	"| %" str(TABLE_COL1_WIDTH) "s "
			"| %" str(TABLE_COL2_WIDTH) "s "
			"| \n"	
			"| %" str(TABLE_COL1_WIDTH) "s "
			"| %" str(TABLE_COL2_WIDTH) "s "
			"|\n",
			"Fork",
			forks[i] == -1 ? "On table" : names[forks[i]],
			names[i],
			state_str[states[i]]
		);
	}

	print_state_table_line();
}

/* Locks the data and prints the state */
void print_state()
{
	pthread_mutex_lock(&mutex_data);
	print_state_no_lock();
	pthread_mutex_unlock(&mutex_data);
}

/* Returns the number of tantrums currently being thrown */
int tantrums()
{
	int i;
	int count = 0;

	for(i = 0; i < NUM_PHILOSOPHERS; i++) {
		if(states[i] == TANTRUM) {
			count++;
		}
	}
	return count;
}

/* Sets state to thinking and waits */
void think(int my_id)
{
	states[my_id] = THINKING;
	print_state();
	#ifdef FAST_MODE
		usleep(
	#else
		sleep(
	#endif
		rand() % 20 + 1);
}

/* Sets state to eating and waits */
void eat(int my_id)
{
	states[my_id] = EATING;
	print_state();
	#ifdef FAST_MODE
		usleep(
	#else
		sleep(
	#endif
		rand() % 8 + 2);
}

/* Checks if forks are available. If no forks are, then wait on cond_forkdown,
 * which fires when another philosopher puts down a fork. If waiting for more
 * then PATIENCE seconds, throw a tantrum, which gives this philosopher
 * priority.
 */
void get_forks(int my_id)
{
	const int left_fork = my_id;
	const int right_fork = (left_fork + 1) % NUM_PHILOSOPHERS;
	int wait_start = time(NULL);

	pthread_mutex_lock(&mutex_data);

	states[my_id] = WAITING;
	print_state_no_lock();

	while(1) {
		if((!tantrums() || states[my_id] == TANTRUM) 
				&& forks[left_fork] == -1 
				&& forks[right_fork] == -1) {
			forks[left_fork] = my_id;
	 		forks[right_fork] = my_id;
			break;
		}

		if(time(NULL) - wait_start > PATIENCE) {
			states[my_id] = TANTRUM;
			print_state_no_lock();
		}

		pthread_cond_wait(&cond_forkdown, &mutex_data);
	}

	pthread_mutex_unlock(&mutex_data);
}

/* Lock the data and put down my forks */
void put_forks(int my_id)
{
	const int left_fork = my_id;
	const int right_fork = (left_fork + 1) % NUM_PHILOSOPHERS;
	
	pthread_mutex_lock(&mutex_data);
	states[my_id] = PUT_FORKS;
	print_state_no_lock();
	forks[left_fork] = -1;
	forks[right_fork] = -1;
	print_state_no_lock();
	pthread_mutex_unlock(&mutex_data);
	pthread_cond_broadcast(&cond_forkdown);
}

/* Main think/get/eat/put loop */
void *philosopher(void *arg)
{
	int my_id = *((int *)arg);
	while(1) {
		think(my_id);
		get_forks(my_id);
		eat(my_id);
		put_forks(my_id);
	}
}

/* Main */
int main()
{
	int i;
	pthread_t philosophers[NUM_PHILOSOPHERS];
	int phil_id[NUM_PHILOSOPHERS];

	srand(time(NULL));

	print_state_table_line();	
    
	/* Initialize the state */
	for(i = 0; i < NUM_PHILOSOPHERS; i++) {
 		forks[i] = -1;
		states[i] = THINKING;
	}
	
	/* Soup's on! */
	for(i = 0; i < NUM_PHILOSOPHERS; i++) {
		phil_id[i] = i;
		pthread_create(&(philosophers[i]), NULL, philosopher, &(phil_id[i]));
	}

	pause();
	return 0;
}
