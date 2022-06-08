#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include "part3.h"
#include "main.h"
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include<fcntl.h>

/*
	- DO NOT USE SLEEP FUNCTION
	- Define every helper function in .h file
	- Use Semaphores for synchronization purposes
 */


/**
* Declare semaphores here so that they are available to all functions.
*/
// sem_t* example_semaphore;
int totalCars ;
int * total_people_in_road ;
int * people_that_have_left ;
sem_t * sem_road_left ;
sem_t * sem_road_right ;
sem_t * change_signal ;
int * people_in_lane[4] ;
int current_green ;
char  encodings_direction[4][100] ;
char encodings_lane[2][100] ;

/**
 * Do any initial setup work in this function. You might want to
 * initialize your semaphores here.
 */
void initializeP3() {
	// example_semaphore = (sem_t*) malloc(sizeof(sem_t));
	totalCars = 0 ;
	total_people_in_road = (int*)malloc(4 * sizeof(int));
	people_that_have_left = (int*)malloc(4 * sizeof(int));
	sem_road_left = (sem_t*)malloc(4 * sizeof(sem_t)) ;
	sem_road_right = (sem_t*)malloc(4 * sizeof(sem_t)) ;
	change_signal = (sem_t*) malloc(sizeof(sem_t)) ;

	current_green = 2 ;
	sem_init(change_signal, 0, 0);

	for(int i=0; i<4; i++)
	{
		people_in_lane[i] = (int*)malloc(2 * sizeof(int));
		for(int j=0; j<2; j++)
		{
			people_in_lane[i][j] = 0 ;
		}
		total_people_in_road[i] = 0 ;
		people_that_have_left[i] = 0 ;
		sem_init( (sem_road_left+i), 0, 0);
		sem_init( (sem_road_right+i), 0, 0);
	}

	strcpy(encodings_direction[0], "NORTH" ) ;
	strcpy(encodings_direction[1], "SOUTH" ) ;
	strcpy(encodings_direction[2], "EAST" ) ;
	strcpy(encodings_direction[3], "WEST" ) ;
	strcpy(encodings_lane[0], "LEFT" ) ;
	strcpy(encodings_lane[1],"RIGHT" ) ;

}


/**
 * This is the function called for each car thread. You can check
 * how these functions are used by going over the test3 function
 * in main.c
 * If there is a car going from SOUTH to NORTH, from lane LEFT,
 * print
 * SOUTH NORTH LEFT
 * Also, if two cars can simulateneously travel in the two lanes,
 * first print all the cars in the LEFT lane, followed by all the
 * cars in the right lane
 *
 * Input: *argu is of type struct argumentP3 defined in main.h
 */
void * goingFromToP3(void *argu){
	// Some code to help in understanding argu
    // struct argumentP3* car = (struct argumentP3*) argu;
    // enum DIRECTION from = car->from;
    // ...
	// LEFT = 0, RIGHT = 1
	// SOUTH = 1, WEST = 3, NORTH = 0, EAST = 2
	struct argumentP3 * temp = (struct argumentP3 *) argu ;
	enum DIRECTION my_from = temp -> from ;
	enum DIRECTION my_to = temp -> to ;
	enum LANE  my_lane = temp -> lane ;
	int id = temp -> user_id ;

	int straight ;
	int turn_left ;
	if( abs(my_from-my_to) == 1 )
	{
		straight = 1 ;
	}
	else
	{
		straight = 0 ;
	}

	if(straight==0 && my_lane == 0)
	{
		turn_left = 1 ;
	}
	else
	{
		turn_left = 0 ;
	}

	if(people_in_lane[my_from][my_lane] == 0 && turn_left == 1)
	{
		printf("%s %s %s\n", encodings_direction[my_from], encodings_direction[my_to], encodings_lane[my_lane]) ;
		return NULL;
	}

	people_in_lane[my_from][my_lane]++ ;
	totalCars ++ ;

	if(my_lane == 0)
	{
		sem_wait(sem_road_left+my_from) ;
	}
	else
	{
		sem_wait(sem_road_right+my_from) ;
	}

	people_in_lane[my_from][my_lane]-- ;
	totalCars -- ;
	people_that_have_left[my_from] ++ ;

	printf("%s %s %s\n", encodings_direction[my_from], encodings_direction[my_to], encodings_lane[my_lane]) ;

	if(people_that_have_left[my_from] == 5 || (people_in_lane[my_from][0] == 0 && people_in_lane[my_from][1]==0) )
	{
		sem_post(change_signal) ;
	}
	else if(my_lane == 0 && people_in_lane[my_from][0] != 0)
	{
		sem_post(sem_road_left+my_from) ;
	}
	else if(my_lane == 0 && people_in_lane[my_from][0] == 0 )
	{
		sem_post(sem_road_right+my_from) ;
	}
	else if(my_lane == 1 && people_in_lane[my_from][1] != 0)
	{
		sem_post(sem_road_right+my_from) ;
	}

	return NULL ;

}


/**
 * startP3 is called once all cars have been initialized. The logic of the traffic signals
 * will go here
 */
void startP3(){
	sleep(1);

	while(1)
	{
		if(totalCars == 0 )
		{
			return ;
		}

		current_green = 2 ;
		if(people_in_lane[current_green][0] !=0 || people_in_lane[current_green][1] != 0)
		{
			if(people_in_lane[current_green][0] != 0 )
			{
				sem_post(sem_road_left+current_green) ;
			}
			else
			{
				sem_post(sem_road_right+current_green) ;
			}

			sem_wait(change_signal) ;
		}

		current_green = 1 ;
		if(people_in_lane[current_green][0] !=0 || people_in_lane[current_green][1] != 0)
		{
			if(people_in_lane[current_green][0] != 0 )
			{
				sem_post(sem_road_left+current_green) ;
			}
			else
			{
				sem_post(sem_road_right+current_green) ;
			}

			sem_wait(change_signal) ;
		}

		current_green = 3 ;
		if(people_in_lane[current_green][0] !=0 || people_in_lane[current_green][1] != 0)
		{
			if(people_in_lane[current_green][0] != 0 )
			{
				sem_post(sem_road_left+current_green) ;
			}
			else
			{
				sem_post(sem_road_right+current_green) ;
			}

			sem_wait(change_signal) ;
		}

		current_green = 0 ;
		if(people_in_lane[current_green][0] !=0 || people_in_lane[current_green][1] != 0)
		{
			if(people_in_lane[current_green][0] != 0 )
			{
				sem_post(sem_road_left+current_green) ;
			}
			else
			{
				sem_post(sem_road_right+current_green) ;
			}

			sem_wait(change_signal) ;
		}
	}
}