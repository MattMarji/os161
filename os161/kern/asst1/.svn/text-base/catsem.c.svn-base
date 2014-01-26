/*
 * catsem.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use SEMAPHORES to solve the cat syncronization problem in 
 * this file.
 */


/*
 * 
 * Includes
 *
 */
#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>


/*
 * 
 * Constants
 *
 */

/*
 * Number of food bowls.
 */

#define NFOODBOWLS 2

/*
 * Number of cats.
 */

#define NCATS 6

/*
 * Number of mice.
 */

#define NMICE 2

	//Create the semaphore!
	struct semaphore *resources;

/*
 Need 2 arrays: to track/store iteration number
 - One to track how many times each cat ate
 - One to track how many times each mouse ate
*/

int cats_iteration[6] = {0,0,0,0,0,0};
int mice_iteration[2] = {0,0};

/* 
 Need 2 Variables to track status of bowls (locks):
 Status:
 0 - Bowl is available
 1 - Bowl occupied by Cat
 2 - Bowl occupied by Mouse
*/

int status_bowl1 = 0;
int status_bowl2 = 0;

/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
sem_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}

/*
 * catsem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
catsem(void * unusedpointer, 
       unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) catnumber;
	
	P(resources); // take a resource (bowl) right away if available!
	while (cats_iteration[catnumber] < 4){ //only continue if the cat has not eaten 4x
	
        //* Note: Spinlocks are for multiprocessors only.
		//while (status_bowl1 == 2 || status_bowl2 == 2){ //If a mouse is currently at a bowl, wait until completion.
			
			//WAIT - SPIN
		//}
		
		//Once we get here, we are NOT waiting anymore!
		
		if (status_bowl1 == 0 && status_bowl2 != 2){ // if bowl available and mouse is not at other bowl.
			//Change status of bowl 1 
			status_bowl1 = 1;
						
			//Time to eat!!
			sem_eat("cat", catnumber, 1, cats_iteration[catnumber]); 
			
			//Update bowl status now that we're done eating.
			status_bowl1 = 0;
			
			//iterate++, release resource.
			cats_iteration[catnumber] = cats_iteration[catnumber] + 1;
						
		
		}
		else if (status_bowl2 == 0 && status_bowl1 != 2) { //if bowl available and mouse is not at other bowl
			//Change status of bowl 2
			status_bowl2 = 1;
			
			//Time to eat!!
			sem_eat("cat", catnumber, 2, cats_iteration[catnumber]); 
			
			//Update bowl status now that we're done eating.
			status_bowl2 = 0;
			
			//iterate++, release resource.
			cats_iteration[catnumber] = cats_iteration[catnumber] + 1;
			
		}
		
		else{
		kprintf("Don't go here cats \n");
		}

	
	}
	
	V(resources);
	//Now that we are done, we should check if we need to kill the thread.
	if (cats_iteration[catnumber] >= 4){
	
	//kill thread here.
	
	}
}
        

/*
 * mousesem()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long mousenumber: holds the mouse identifier from 0 to 
 *              NMICE - 1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using semaphores.
 *
 */

static
void
mousesem(void * unusedpointer, 
         unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) unusedpointer;
        (void) mousenumber;
		
	P(resources); // take a resource (bowl) right away if available!
	while (mice_iteration[mousenumber] < 4){ //only continue if the cat has not eaten 4x
	
	//while (status_bowl1 == 1 || status_bowl2 == 1){ //If a cat is currently at a bowl, wait until completion.
		
		//WAIT - SPIN!
	//}
	
	//Once we get here, we are NOT waiting anymore!
	
	if (status_bowl1 == 0 && status_bowl2 != 1){ // If one bowl free, and cat is not at the other.
		//Change status of bowl 1 
		status_bowl1 = 2;
		
		//Time to eat!!
		sem_eat("mouse", mousenumber, 1, mice_iteration[mousenumber]); 
		
		//Update bowl status now that we're done eating.
		status_bowl1 = 0;
		
		//iterate++, release resource.
		mice_iteration[mousenumber] = mice_iteration[mousenumber] + 1;
	}
	else if (status_bowl2 == 0 && status_bowl1 != 1){ // If one bowl free, and cat is not at the other.
		//Change status of bowl 2
		status_bowl2 = 2;
		
		//Time to eat!!
		sem_eat("mouse", mousenumber, 2, mice_iteration[mousenumber]); 
		
		//Update bowl status now that we're done eating.
		status_bowl2 = 0;
		
		//iterate++, release resource.
		mice_iteration[mousenumber] = mice_iteration[mousenumber] + 1;
	}
	else{
	kprintf("You better not come here mice! \n");
	}

}
	V(resources);
	
	//Now that we are done, we should check if we need to kill the thread.
	if (mice_iteration[mousenumber] >= 4){
	
	//kill thread here.
	
	}
}


/*
 * catmousesem()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catsem() and mousesem() threads.  Change this 
 *      code as necessary for your solution.
 */

int
catmousesem(int nargs,
            char ** args)
{
	
	//Initialize the semaphore.
	resources = sem_create("bowls", 2);
	
	

        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
   
        /*
         * Start NCATS catsem() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catsem Thread", 
                                    NULL, 
                                    index, 
                                    catsem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catsem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }
        
        /*
         * Start NMICE mousesem() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mousesem Thread", 
                                    NULL, 
                                    index, 
                                    mousesem, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mousesem: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        return 0;
}


/*
 * End of catsem.c
 */
