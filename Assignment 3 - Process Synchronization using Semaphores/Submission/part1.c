#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include "part1.h"
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
const int MAX_NUM_FLOORS = 20;
int num_floors ;						// total number of floors
int capacity ;							// capacity of elevator
int people_in_elevator ;				// number of people currently in elevator
int current_floor = 0 ;					// current floor of elevator
sem_t* people_outside ;	// array people outside each floor (semaphore)
sem_t* people_inside ;		// array people inside elevator on each floor
int state ;								// elevator going up or down, 1 = up, 2 = down
sem_t* move ;
int * people_outside_floor ;
int * destination_floor ;
/**
 * Do any initial setup work in this function. You might want to
 * initialize your semaphores here. Remember this is C and uses Malloc for memory allocation.
 *
 * numFloors: Total number of floors elevator can go to. numFloors will be smaller or equal to MAX_NUM_FLOORS
 * maxNumPeople: The maximum capacity of the elevator
 *
 */
void initializeP1(int numFloors, int maxNumPeople) {
	// example_semaphore = (sem_t*) malloc(sizeof(sem_t));
	num_floors = numFloors ;
	capacity = maxNumPeople ;
	state = 1 ;
	people_in_elevator = 0 ;
	people_outside_floor = (int*)malloc(MAX_NUM_FLOORS * sizeof(int));
	destination_floor = (int*)malloc(MAX_NUM_FLOORS * sizeof(int));
	move = (sem_t*) malloc(sizeof(sem_t)) ;
	sem_init(move, 0, 0);

	people_outside = (sem_t*)malloc(MAX_NUM_FLOORS * sizeof(sem_t)) ;
	people_inside= (sem_t*)malloc(MAX_NUM_FLOORS * sizeof(sem_t)) ;

	for(int i = 0; i<MAX_NUM_FLOORS; i++)
	{
		people_outside_floor[i] = 0 ;
	}

	for(int i=0; i<MAX_NUM_FLOORS; i++)
	{
		sem_init( (people_outside+i), 0, 0);
		sem_init( (people_inside+i), 0, 0);
	}

	return;
}


/**
 * Every passenger will call this function when
 * he/she wants to take the elevator. (Already
 * called in main.c)
 *
 * This function should print info "id from to" without quotes,
 * where:
 * 	id = id of the user (would be 0 for the first user)
 * 	from = source floor (from where the passenger is taking the elevator)
 * 	to = destination floor (floor where the passenger is going)
 *
 * info of a user x_1 getting off the elevator before a user x_2
 * should be printed before.
 *
 * Suppose a user 1 from floor 1 wants to go to floor 4 and
 * a user 2 from floor 2 wants to go to floor 3 then the final print statements
 * will be
 * 2 2 3
 * 1 1 4
 *
 */
void* goingFromToP1(void *arg) {

	//printf("hoja\n") ;
	struct argument * temp = (struct argument *) arg ;

	int starting_floor = temp -> from ;
	int final_floor = temp -> to ;
	int id = temp -> id ;

	people_outside_floor[starting_floor] ++ ;

	//printf("me yahan hun abhi %d %d %d\n", id, starting_floor, final_floor) ;

	sem_wait( (people_outside+starting_floor) ) ;

	//printf("lift me charh raha hun %d %d %d\n", id, starting_floor, final_floor) ;

	people_in_elevator ++ ;
	people_outside_floor[starting_floor] -- ;
	destination_floor[final_floor] ++ ;

	if(people_outside_floor[starting_floor] == 0 || people_in_elevator == capacity)
	{
		sem_post(move) ;
	}
	else
	{
		sem_post( (people_outside+starting_floor) ) ;
	}


	sem_wait(people_inside+final_floor) ;
	people_in_elevator -- ;
	destination_floor[final_floor] -- ;
	printf("%d %d %d\n", id, starting_floor, final_floor) ;

	if(destination_floor[final_floor] == 0)
	{
		sem_post(move) ;
	}
	else
	{
		sem_post(people_inside+final_floor) ;
	}

	// printf("%d\n", id) ;

	return NULL;
}

/*If you see the main file, you will get to
know that this function is called after setting every
passenger.

So use this function for starting your elevator. In
this way, you will be sure that all passengers are already
waiting for the elevator.
*/
void startP1(){
	sleep(1);

	current_floor = 0;

	//printf(" %d wale floor aja\n", current_floor) ;
	sem_post(people_outside+current_floor) ;
	sem_wait(move) ;

	while(1)
	{
		//printf("current floor = %d , capacity = %d\n", current_floor, people_in_elevator) ;
		int check = 0 ;

		if (people_in_elevator == 0)
		{
			for(int i=0; i<MAX_NUM_FLOORS; i++)
			{
				if(people_outside_floor[i] !=0 )
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
			//printf("bc khali pari hai\n") ;
			return ;
		}


		if(current_floor == 20 && state == 1)
		{
			state = 2 ;
		}
		else if (current_floor == 0 && state == 2)
		{
			state = 1 ;
		}

		if(state == 1)
		{
			current_floor = current_floor + 1 ;
		}
		else if(state == 2)
		{
			current_floor = current_floor - 1 ;
		}

		if(people_outside_floor[current_floor] == 0 && destination_floor[current_floor] == 0)
		{
			//printf("skipping this floor %d\n", current_floor) ;
			continue ;
		}

		if(people_outside_floor[current_floor] == 0 && destination_floor[current_floor] != 0)
		{
			//printf(" %d floor wale nikalja\n", current_floor) ;
			sem_post(people_inside+current_floor) ;
			sem_wait(move) ;
			continue ;
		}

		if(people_outside_floor[current_floor] != 0 && destination_floor[current_floor] == 0 && people_in_elevator != capacity)
		{
			//printf(" %d wale floor aja\n", current_floor) ;
			sem_post(people_outside+current_floor) ;
			sem_wait(move) ;
			continue ;
		}

		if(people_outside_floor[current_floor] != 0 && destination_floor[current_floor] != 0)
		{
			//printf(" %d floor wale nikalja\n", current_floor) ;
			sem_post(people_inside+current_floor) ;
			sem_wait(move) ;

			//printf(" %d wale floor aja\n", current_floor) ;
			sem_post(people_outside+current_floor) ;
			sem_wait(move) ;
			continue ;
		}

	}
}
