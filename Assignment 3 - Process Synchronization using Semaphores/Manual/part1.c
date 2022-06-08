#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include "part1.h"
#include "main.h"

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
}