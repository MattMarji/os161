/*
 * catlock.c
 *
 * 30-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: Please use LOCKS/CV'S to solve the cat syncronization problem in 
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

/*
 Need 2 arrays: to track/store iteration number
 - One to track how many times each cat ate
 - One to track how many times each mouse ate
*/

/* 
 Need 2 Variables to track status of bowls (locks):
 Status:
 0 - Bowl is available
 1 - Bowl occupied by Cat
 2 - Bowl occupied by Mouse
*/

/*
 * 
 * Function Definitions
 * 
 */

/* who should be "cat" or "mouse" */
static void
lock_eat(const char *who, int num, int bowl, int iteration)
{
        kprintf("%s: %d starts eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
        clocksleep(1);
        kprintf("%s: %d ends eating: bowl %d, iteration %d\n", who, num, 
                bowl, iteration);
}

/*
 * catlock()
 *
 * Arguments:
 *      void * unusedpointer: currently unused.
 *      unsigned long catnumber: holds the cat identifier from 0 to NCATS -
 *      1.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
catlock(void * unusedpointer, 
        unsigned long catnumber)
{
        /*
         * Avoid unused variable warnings.
         */
    
        /* Pseudo Code to implement for Cat:
         
         1) Check how many times cat has eaten
            - IF THE FUCKER IS FAT (more that 4) kill thread [thread exit?]
            - IF NOT continue to 2)
         2) Lock condition code
         3) [within condition code] Check status of bowls
            3a. Check both bowls if mouse is eating, if true - spinlock
            3b. if false, check if a bowl is avail - if true - assign cat => 4
            3c. if false, spinlock
         4) Enter a Bowl [applies for 1 and 2], unlock condition code,  Lock Bowl code
            4a. set status of bowl to 1 (cat)
            4b. call lock_eat()
            4c. increment iteration
            4d. set status to 0
            4e. unlock Bowl code
         */

        (void) unusedpointer;
        (void) catnumber;
}
	

/*
 * mouselock()
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
 *      Write and comment this function using locks/cv's.
 *
 */

static
void
mouselock(void * unusedpointer,
          unsigned long mousenumber)
{
        /*
         * Avoid unused variable warnings.
         */
    
        /* Pseudo Code to implement for Mouse:
     
         1) Check how many times mouse has eaten
            - IF THE FUCKER IS FAT (more that 4) kill thread [thread exit?]
            - IF NOT continue to 2)
         2) Lock condition code
         3) [within condition code] Check status of bowls
            3a. Check both bowls if there is a Cat eating, if true - spinlock
            3b. if false, check if a bowl is avail - if true - assign mouse => 4
            3c. if false, spinlock
         4) Enter a Bowl [applies for 1 and 2], unlock condition code,  Lock Bowl code
            4a. set status of bowl to 3 (mouse)
            4b. call lock_eat()
            4c. increment iteration
            4d. set status to 0
            4e. unlock Bowl code
     */
    
        (void) unusedpointer;
        (void) mousenumber;
}


/*
 * catmouselock()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up catlock() and mouselock() threads.  Change
 *      this code as necessary for your solution.
 */

int
catmouselock(int nargs,
             char ** args)
{
        int index, error;
   
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
    
        //* Create BOWL Locks -- DONE
    
        //* Create Lock for Condition Code -- DONE
    
        //* Set global array counter (Iterations) to 0s (MAYBE) Assuming
        //* user wants to reset the variables
   
        /*
         * Start NCATS catlock() threads.
         */

        for (index = 0; index < NCATS; index++) {
           
                error = thread_fork("catlock thread", 
                                    NULL, 
                                    index, 
                                    catlock, 
                                    NULL
                                    );
                
                /*
                 * panic() on error.
                 */

                if (error) {
                 
                        panic("catlock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        /*
         * Start NMICE mouselock() threads.
         */

        for (index = 0; index < NMICE; index++) {
   
                error = thread_fork("mouselock thread", 
                                    NULL, 
                                    index, 
                                    mouselock, 
                                    NULL
                                    );
      
                /*
                 * panic() on error.
                 */

                if (error) {
         
                        panic("mouselock: thread_fork failed: %s\n", 
                              strerror(error)
                              );
                }
        }

        return 0;
}

/*
 * End of catlock.c
 */
