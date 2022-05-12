#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#include "museumsim.h"

//
// In all of the definitions below, some code has been provided as an example
// to get you started, but you do not have to use it. You may change anything
// in this file except the function signatures.
//


struct shared_data {
	// Add any relevant synchronization constructs and shared state here.
	pthread_mutex_t ticket_mutex;
	pthread_cond_t guide_enter; //visitors wait for guides' entering 
	pthread_cond_t guide_leave; //guides wait to leave
	pthread_cond_t visitor_arrive; //visitors arrive the enterance of museum
	pthread_cond_t guide_wait; //there have been two guides inside so other guides have to wait outside
    pthread_cond_t visitor_leave; //visitor have finished their tourism and leave
    pthread_barrier_t guide_leave_together; // guides need to leave together

	int tickets;
	int visitors_waiting; //visitors who are waiting to tour
	int guides_inside; //guides who are in the museum already
	int visitors_inside; //visitors who are in the museum already
	int visitors_admitted; //visitors who are admitted by the guides
	int guides_finish; //guide who have admitted 10 visitors and are waiting to leave
    int visitor_remaining;  //a flag used to test whether there are still visitors inside

};

static struct shared_data shared;


/**
 * Set up the shared variables for your implementation.
 * 
 * `museum_init` will be called before any threads of the simulation
 * are spawned.
 */
void museum_init(int num_guides, int num_visitors)
{
	/*initializations, all declarations are shown above*/
	pthread_mutex_init(&shared.ticket_mutex, NULL);
	pthread_cond_init(&shared.visitor_arrive, NULL);
	pthread_cond_init(&shared.guide_enter, NULL);
	pthread_cond_init(&shared.guide_leave, NULL);
	pthread_cond_init(&shared.guide_wait, NULL);
    pthread_cond_init(&shared.visitor_leave, NULL);
    pthread_barrier_init(&shared.guide_leave_together, NULL, MIN(GUIDES_ALLOWED_INSIDE, num_guides));

	shared.tickets = MIN(VISITORS_PER_GUIDE * num_guides, num_visitors);
	shared.visitors_waiting = 0;
	shared.guides_inside = 0;
	shared.visitors_inside = 0;
	shared.visitors_admitted = 0;
	shared.guides_finish = 0;
    shared.visitor_remaining = 1;
    
}


/**
 * Tear down the shared variables for your implementation.
 * 
 * `museum_destroy` will be called after all threads of the simulation
 * are done executing.
 */
void museum_destroy()
{
	/*deletions, all declarations are shown above*/
	pthread_mutex_destroy(&shared.ticket_mutex);
	pthread_cond_destroy(&shared.visitor_arrive);
	pthread_cond_destroy(&shared.guide_enter);
	pthread_cond_destroy(&shared.guide_leave);
	pthread_cond_destroy(&shared.guide_wait);
    pthread_cond_destroy(&shared.visitor_leave);
    pthread_barrier_destroy(&shared.guide_leave_together);
}


/**
 * Implements the visitor arrival, touring, and leaving sequence.
 */
void visitor(int id)
{
	visitor_arrives(id);
	pthread_mutex_lock(&shared.ticket_mutex);
	{	
		/*all tickets are sold out and visitor has to leave*/
		if(shared.tickets == 0){ 
			visitor_leaves(id);
			pthread_mutex_unlock(&shared.ticket_mutex);
			return;
		} 
		/*we have tickets and the visitors buy them, so they can wait */
		else{
		shared.tickets -= 1;
		shared.visitors_waiting += 1;
		pthread_cond_broadcast(&shared.visitor_arrive);
		}

		/*there is no guide inside, so we wait guide to enter*/
		while (shared.guides_inside == 0 ){
        //while (shared.guides_inside == 0 && ){
			pthread_cond_wait(&shared.guide_enter, &shared.ticket_mutex);
		}
	}
	pthread_mutex_unlock(&shared.ticket_mutex);

	visitor_tours(id);

	/*visitors gradually leave until there is no visitor inside, so guide can leave as well*/
	pthread_mutex_lock(&shared.ticket_mutex);
	{
        pthread_cond_signal(&shared.visitor_leave);
		visitor_leaves(id);
		shared.visitors_inside -= 1;
		if (shared.visitors_inside == 0){
			pthread_cond_broadcast(&shared.guide_leave);
		}
	}
	pthread_mutex_unlock(&shared.ticket_mutex);
	return;
}

/**
 * Implements the guide arrival, entering, admitting, and leaving sequence.
 */
void guide(int id)
{
	
	pthread_mutex_lock(&shared.ticket_mutex);
    {

        guide_arrives(id);

		/*guides can leave immediately wihout entering if there is no tickets or waiting visitors*/
		if(shared.tickets == 0 && shared.visitors_waiting == 0){
			pthread_mutex_unlock(&shared.ticket_mutex);
			return;
		}

		/*guides need to wait if there are guides waiting to leave or 2 guides inside*/
		while(shared.guides_finish != 0 || shared.guides_inside == GUIDES_ALLOWED_INSIDE){
			pthread_cond_wait(&shared.guide_wait, &shared.ticket_mutex);
		}
	
		guide_enters(id);
		shared.guides_inside +=1;

		/*admit visitors unless there is no tickets or no visitors are waiting*/
		while(shared.visitors_admitted < VISITORS_PER_GUIDE){
			if(shared.tickets == 0 && shared.visitors_waiting == 0){
				break;
			}
			/*wait for visitors' arriving*/
			while(shared.visitors_waiting == 0){
				pthread_cond_wait(&shared.visitor_arrive, &shared.ticket_mutex);
				//use a flag to test whether there is visitor remaining
				if(shared.visitors_waiting == 0 && shared.tickets == 0){
					shared.visitor_remaining = 0;
					break;
				}
			}
			
			if(shared.visitor_remaining == 0){
				break;
			}

			guide_admits(id);
            
			shared.visitors_waiting -=1;
			shared.visitors_inside +=1;
			shared.visitors_admitted +=1;
			pthread_cond_broadcast(&shared.guide_enter);
		}

		shared.guides_finish +=1;
        //pthread_cond_broadcast(&shared.guide_leave);

		/*ongly all visitors have left and the second visitor finishes, they can leave together*/
		while(shared.visitors_inside != 0 || shared.guides_finish < shared.guides_inside){
			pthread_cond_wait(&shared.guide_leave, &shared.ticket_mutex);
		}

		/*if there is no visitors inside, guide can leave*/
        if(shared.visitors_inside ==0){
            shared.guides_finish -=1;
		    shared.guides_inside -=1;
            //guide_leaves(id);
		    pthread_cond_broadcast(&shared.guide_wait);
            guide_leaves(id);

        }

		/*guide leave together*/
        if(shared.guides_inside > 1){
		    pthread_barrier_wait(&shared.guide_leave_together);
	    }

		
	}
	pthread_mutex_unlock(&shared.ticket_mutex);
	return;
}
