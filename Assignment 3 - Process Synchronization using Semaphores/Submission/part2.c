#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include "part2.h"
#include "main.h"
#include <stdlib.h>

/*
	- DO NOT USE SLEEP FUNCTION
	- Define every helper function in .h file
	- Use Semaphores for synchronization purposes
 */


/**
* Declare semaphores here so that they are available to all functions.
*/
// sem_t* example_semaphore;
const int INTER_ARRIVAL_TIME = 5;
const int NUM_TRAINS = 5;
int num_of_stations ;
int * current_station ;
int * people_in_each_train ;
int * people_at_each_station ;
int * people_dest_each_station ;
int * train_at_station ;
sem_t * move ;
sem_t * people_outside ;
sem_t * people_inside ;
sem_t * station_check ;

/**
 * Do any initial setup work in this function. You might want to
 * initialize your semaphores here. Remember this is C and uses Malloc for memory allocation.
 *
 * numStations: Total number of stations. Will be >= 5. Assume that initially
 * the first train is at station 1, the second at 2 and so on.
 * maxNumPeople: The maximum number of people in a train
 */
void initializeP2(int numStations, int maxNumPeople) {
	// example_semaphore = (sem_t*) malloc(sizeof(sem_t));
	num_of_stations = numStations ;

	people_in_each_train = (int*)malloc(5 * sizeof(int));
	people_at_each_station = (int*)malloc(numStations * sizeof(int));
	people_dest_each_station = (int*)malloc(numStations * sizeof(int));
	current_station = (int*)malloc(5 * sizeof(int));
	train_at_station = (int*)malloc(numStations * sizeof(int));

	move = (sem_t*)malloc(5 * sizeof(sem_t)) ;
	people_outside = (sem_t*)malloc(numStations * sizeof(sem_t)) ;
	people_inside = (sem_t*)malloc(numStations * sizeof(sem_t)) ;
	station_check = (sem_t*)malloc(numStations * sizeof(sem_t)) ;

	for(int i=0; i<5; i++)
	{
		people_in_each_train[i] = 0 ;
		train_at_station[i] = i ;
		current_station[i] = i ;
		sem_init( (move+i) , 0, 0);
		sem_init( (station_check+i) , 0, 0);
		//sem_init( (station+i) , 0, 1);
	}

	for(int i=0; i<numStations; i++)
	{
		if(i>=5)
		{
			sem_init( (station_check+i) , 0, 1);
			train_at_station[i] = -1 ;
		}

		//sem_init( (station_check+i) , 0, 0);
		people_at_each_station[i] = 0 ;
		people_dest_each_station[i] = 0 ;
		sem_init( (people_outside+i) , 0, 0);
		sem_init( (people_inside+i) , 0, 0);
	}

	return ;
}

/**
	This function is called by each user.

 * Print in the following format:
 * If a user borads on train 0, from station 0 to station 1, and another boards
 * train 2 from station 2 to station 4, then the output will be
 * 0 0 1
 * 2 2 4
 */
void * goingFromToP2(void * user_data) {

	struct argument * temp = (struct argument *) user_data ;
	int starting_station = temp -> from ;
	int final_station = temp -> to ;
	int id = temp -> id ;

	people_at_each_station[starting_station]++ ;
	sem_wait( (people_outside+starting_station) ) ;

	int my_train = train_at_station[starting_station] ;

	people_in_each_train[my_train] ++ ;
	people_at_each_station[starting_station]-- ;
	people_dest_each_station[final_station] ++ ;

	if(people_at_each_station[starting_station] == 0 || people_in_each_train[starting_station] == 50)
	{
		sem_post( (move+my_train) ) ;
	}
	else
	{
		sem_post( (people_outside+starting_station) ) ;
	}

	sem_wait(people_inside+final_station) ;
	people_in_each_train[my_train] -- ;
	people_dest_each_station[final_station] -- ;
	printf("%d %d %d\n", my_train, starting_station, final_station) ;

	if(people_dest_each_station[final_station] == 0)
	{
		sem_post( (move+my_train) ) ;
	}
	else
	{
		sem_post(people_inside+final_station) ;
	}

	return NULL ;
}

/* Function to manage a single train */
void * train(void * trainNumber)
{
	int * temp = (int *) trainNumber ;
	int my_train_number = *temp ;

	current_station[my_train_number] = my_train_number ;

	int my_current_station = my_train_number ;
	int next_station = ( my_current_station +1 ) % num_of_stations ;

	sem_post( (people_outside+my_current_station) ) ;
	sem_wait( (move+my_train_number) ) ;

	while(1)
	{
		int check = 0 ;
		if(people_in_each_train[my_train_number] == 0)
		{
			for(int i=0; i<num_of_stations; i++)
			{
				if(people_at_each_station[i] !=0 )
				{
					check = 1 ;
				}
			}
		}
		else
		{
			check = 1 ;
		}

		if(check == 0)
		{
			train_at_station[ current_station[my_train_number] ] = -1 ;
			break;
		}

		sem_wait( (station_check+next_station) ) ;

		train_at_station[ my_current_station ] = -1 ;
		int previous_station = my_current_station ;
		my_current_station = next_station ;
		current_station[my_train_number] = next_station ;

		next_station = ( my_current_station +1 ) % num_of_stations ;
		train_at_station[ my_current_station ] = my_train_number ;

		if(people_at_each_station[ current_station[my_train_number] ] == 0 && people_dest_each_station[ current_station[my_train_number] ] == 0)
		{
			sem_post( (station_check+my_current_station) ) ;
			continue ;
		}

		if(people_at_each_station[current_station[my_train_number]] == 0 && people_dest_each_station[current_station[my_train_number]] != 0)
		{
			sem_post(people_inside+current_station[my_train_number]) ;
			sem_wait(move + my_train_number) ;
			sem_post( (station_check+previous_station) ) ;
			continue ;
		}

		if(people_at_each_station[current_station[my_train_number]] != 0 && people_dest_each_station[current_station[my_train_number]] == 0 && people_in_each_train[my_train_number] != 50)
		{
			sem_post(people_outside+current_station[my_train_number]) ;
			sem_wait(move + my_train_number) ;
			sem_post( (station_check+previous_station) ) ;
			continue ;
		}

		if(people_in_each_train[ current_station[my_train_number] ] != 0 && people_dest_each_station[current_station[my_train_number]] != 0)
		{
			sem_post(people_inside+current_station[my_train_number]) ;
			sem_wait(move + my_train_number) ;

			sem_post(people_outside+current_station[my_train_number]) ;
			sem_wait(move + my_train_number) ;
			sem_post( (station_check+previous_station) ) ;
			continue ;
		}
	}
}

/* Use this function to start threads for your trains */
void * startP2(){
	sleep(1);

	int * trainid0 = (int*)malloc(1 * sizeof(int));
	trainid0[0] = 0 ;

	int * trainid1 = (int*)malloc(1 * sizeof(int));
	trainid1[0] = 1 ;

	int * trainid2 = (int*)malloc(1 * sizeof(int));
	trainid2[0] = 2 ;

	int * trainid3 = (int*)malloc(1 * sizeof(int));
	trainid3[0] = 3 ;

	int * trainid4 = (int*)malloc(1 * sizeof(int));
	trainid4[0] = 4 ;

	pthread_t tid0;
	pthread_create(&tid0, NULL, train, (void*) trainid0);
	pthread_t tid1;
	pthread_create(&tid1, NULL, train, (void*) trainid1);
	pthread_t tid2;
	pthread_create(&tid2, NULL, train, (void*) trainid2);
	pthread_t tid3;
	pthread_create(&tid3, NULL, train, (void*) trainid3);
	pthread_t tid4;
	pthread_create(&tid4, NULL, train, (void*) trainid4);

	pthread_join(tid0, NULL);
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);
	pthread_join(tid4, NULL);

	return NULL;
}