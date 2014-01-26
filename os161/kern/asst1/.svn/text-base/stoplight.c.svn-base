/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
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


//* Initialize condition variable and locks
struct cv *NWcv;
struct cv *NEcv;
struct cv *SWcv;
struct cv *SEcv;
struct cv *numcars;
struct lock *NWlock;
struct lock *NElock;
struct lock *SWlock;
struct lock *SElock;
struct lock *templock;

/*
 *
 * Constants
 *
 */

// status variables to indicate the availability of regions (0 - avail, 1 - occupied)
int NWregion = 0;
int NEregion = 0;
int SEregion = 0;
int SWregion = 0;
int firstrun = 0;
int carlimit = 0;

/*
 * Number of cars created.
 */

#define NCARS 20


/*
 *
 * Function Definitions
 *
 */

static const char *directions[] = { "N", "E", "S", "W" };

static const char *msgs[] = {
        "approaching:",
        "region1:    ",
        "region2:    ",
        "region3:    ",
        "leaving:    "
};

/* use these constants for the first parameter of message */
enum { APPROACHING, REGION1, REGION2, REGION3, LEAVING };

static void
message(int msg_nr, int carnumber, int cardirection, int destdirection)
{
        kprintf("%s car = %2d, direction = %s, destination = %s\n",
                msgs[msg_nr], carnumber,
                directions[cardirection], directions[destdirection]);
}
 
/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
        
        (void) cardirection;
        (void) carnumber;
        int cardestination;
    
    if (cardirection==0){
        cardestination = 2; 
        message(APPROACHING, carnumber, cardirection, cardestination); // approaching region NORTH.
        
        //proceed to region 1 - NW
        lock_acquire(NWlock); // attempt to acquire the NW region LOCK.
		
        while (NWregion==1){ //if the NW region is occupied, wait.
            cv_wait(NWcv,NWlock); // wait in the queue. Go to sleep.
        }
		//IF YOU GET HERE -> you have taken the NW region lock.
        NWregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        NWregion = 0;

        //proceed to region 2 - SW
		//Now that we have the NW region, attempt to lock the SW region.
        lock_acquire(SWlock); 
        while (SWregion==1){ //If the SW region is taken...
            cv_wait(SWcv,SWlock); //wait in the queue. Go to sleep.
        }
		//IF YOU GET HERE -> You have the NW region AND the SW region!
        SWregion = 1;
        message(REGION2,carnumber,cardirection,cardestination);
        SWregion = 0;

        //call signals
        if (NWcv->count!=0)
			cv_signal(NWcv,NWlock);
        lock_release(NWlock);
		if (SWcv->count!=0)
            cv_signal(SWcv,SWlock);
        
        message(LEAVING,carnumber,cardirection,cardestination);
		//unlock locks
        
        lock_release(SWlock);
        
        
    }
    else if (cardirection==1){
        cardestination = 3;
        message(APPROACHING, carnumber, cardirection, cardestination);
        
        //proceed to region 1 - NE
        lock_acquire(NElock);
        while (NEregion==1){
            cv_wait(NEcv,NElock);
        }
        NEregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        NEregion = 0;
        
        //proceed to region 2 - NW
        lock_acquire(NWlock);
        while (NWregion==1){
            cv_wait(NWcv,NWlock);
        }
        NWregion = 1;
        
        //message
        
        message(REGION2,carnumber,cardirection,cardestination);
        
        //free region
        
        NWregion = 0;
        
        //call signals
        if (NEcv->count!=0)
			cv_signal(NEcv,NElock);
        lock_release(NElock);
		if (NWcv->count!=0)
            cv_signal(NWcv,NWlock);
        
        message(LEAVING,carnumber,cardirection,cardestination);
        //unlock locks
        
        lock_release(NWlock);
        
        
    }
    else if (cardirection==2){
        cardestination = 0;
        message(APPROACHING, carnumber, cardirection, cardestination);
        
        //proceed to region 1 - SE
        lock_acquire(SElock);
        while (SEregion==1){
            cv_wait(SEcv,SElock);
        }
        SEregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        SEregion = 0;
        
        //proceed to region 2 - NE
        lock_acquire(NElock);
        while (NEregion==1){
            cv_wait(NEcv,NElock);
        }
        NEregion = 1;
        
        //message
        
        message(REGION2,carnumber,cardirection,cardestination);
        
        //release
        
        NEregion = 0;

        if (SEcv->count!=0)
			 cv_signal(SEcv,SElock);
        lock_release(SElock);
		if(NEcv->count!=0){
           
            cv_signal(NEcv,NElock);
        }
        message(LEAVING,carnumber,cardirection,cardestination);
        //unlock locks
        
        lock_release(NElock);
        
        
    }
    else if (cardirection==3){
        cardestination = 1;
        message(APPROACHING, carnumber, cardirection, cardestination);
        
        //proceed to region 1 - SW
        lock_acquire(SWlock);
        while (SWregion==1){
            cv_wait(SWcv,SWlock);
        }
        SWregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        SWregion = 0;

        
        //proceed to region 2 - SE
        lock_acquire(SElock);
        while (SEregion==1){
            cv_wait(SEcv,SElock);
        }
        SEregion = 1;
        
        //message
        
        message(REGION2,carnumber,cardirection,cardestination);
        
                SEregion = 0;


        if (SWcv->count!=0)
			cv_signal(SWcv,SWlock);
        lock_release(SWlock);
		if(SEcv->count!=0){
            
            cv_signal(SEcv,SElock);
        }
        
		message(LEAVING,carnumber,cardirection,cardestination);
		
        //unlock locks
        
        lock_release(SElock);
        
        
    }
}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;
        int cardestination;
    
    if (cardirection==0){
        cardestination = 1;
        
        //approach the intersection
        message(APPROACHING, carnumber, cardirection, cardestination);
        
        //proceed to region 1 - NW
        lock_acquire(NWlock);
        while (NWregion==1){
            cv_wait(NWcv,NWlock);
        }      
        NWregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        NWregion = 0;
        
        

        //proceed to region 2 - SW
        lock_acquire(SWlock);
        while (SWregion==1){
            cv_wait(SWcv,SWlock);
        }
		SWregion = 1;
        message(REGION2,carnumber,cardirection,cardestination);
        SWregion = 0;
            if (NWcv->count!=0)
                cv_signal(NWcv,NWlock);
            lock_release(NWlock);
        
        
        //proceed to region 3 - SE
        lock_acquire(SElock);
        while (SEregion==1){
            cv_wait(SEcv,SElock);
        }
		
        SEregion = 1;
        message(REGION3,carnumber,cardirection,cardestination);
        SEregion = 0;
            if(SWcv->count!=0)
                cv_signal(SWcv,SWlock);
            lock_release(SWlock);
        if (SEcv->count!=0){
            cv_signal(SEcv,SElock);
        }
        message(LEAVING,carnumber,cardirection,cardestination);
        lock_release(SElock);
        
        
    }
    else if (cardirection==1){
        cardestination = 2;
        //approach the intersection
        message(APPROACHING, carnumber, cardirection, cardestination);
        
        //proceed to region 1 - NE
        lock_acquire(NElock);
        while (NEregion==1){
            cv_wait(NEcv,NElock);
        }
        NEregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        NEregion = 0;
        
        
        
        //proceed to region 2 - NW
        lock_acquire(NWlock);
        while (NWregion==1){
            cv_wait(NWcv,NWlock);
        }
        NWregion = 1;
        message(REGION2,carnumber,cardirection,cardestination);
        NWregion = 0;
        if (NEcv->count!=0)
            cv_signal(NEcv,NElock);
        lock_release(NElock);
        
        
        
        //proceed to region 3 - SW
        lock_acquire(SWlock);
        while (SWregion==1){
            cv_wait(SWcv,SWlock);
        }
        SWregion = 1;
        message(REGION3,carnumber,cardirection,cardestination);
        SWregion = 0;
        if(NWcv->count!=0)
            cv_signal(NWcv,NWlock);
        lock_release(NWlock);
        if(SWcv->count!=0){
            cv_signal(SWcv,SWlock);
        }

		message(LEAVING,carnumber,cardirection,cardestination);
        lock_release(SWlock);
        
        
    }
    else if (cardirection==2){
        cardestination = 3;
        //approach the intersection
        message(APPROACHING, carnumber, cardirection, cardestination);
        
        //proceed to region 1 - SE
        lock_acquire(SElock);
        while (SEregion==1){
            cv_wait(SEcv,SElock);
        }
        SEregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        SEregion = 0;
        
        
        
        //proceed to region 2 - NE
        lock_acquire(NElock);
        while (NEregion==1){
            cv_wait(NEcv,NElock);
        }
        NEregion = 1;
        message(REGION2,carnumber,cardirection,cardestination);
        NEregion = 0;
        if (SEcv->count!=0)
            cv_signal(SEcv,SElock);
        lock_release(SElock);
        
        
        
        //proceed to region 3 - NW
        lock_acquire(NWlock);
        while (NWregion==1){
            cv_wait(NWcv,NWlock);
        }
        NWregion = 1;
        message(REGION3,carnumber,cardirection,cardestination);
        NWregion = 0;
        if(NEcv->count!=0)
            cv_signal(NEcv,NElock);
        lock_release(NElock);
        
        if(NWcv->count!=0){
            cv_signal(NWcv,NWlock);
        }
        message(LEAVING,carnumber,cardirection,cardestination);
        lock_release(NWlock);
        
        
    }
    else if (cardirection==3){
        cardestination = 0;
        //approach the intersection
        message(APPROACHING, carnumber, cardirection, cardestination);
        
        //proceed to region 1 - SW
        lock_acquire(SWlock);

        while (SWregion==1){
            cv_wait(SWcv,SWlock);
        }
        SWregion = 1;
        message(REGION1,carnumber,cardirection,cardestination);
        SWregion = 0;
        
        
        
        //proceed to region 2 - SE
        lock_acquire(SElock);
        while (SEregion==1){
            cv_wait(SEcv,SElock);
        }
        SEregion = 1;
        message(REGION2,carnumber,cardirection,cardestination);
        SEregion = 0;
        if (SWcv->count!=0)
            cv_signal(SWcv,SWlock);
        lock_release(SWlock);
        
        
       
        //proceed to region 3 - NE
        lock_acquire(NElock);
        while (NEregion==1){
            cv_wait(NEcv,NElock);
        }
        NEregion = 1;
        message(REGION3,carnumber,cardirection,cardestination);
        NEregion = 0;
        if(SEcv->count!=0)
            cv_signal(SEcv,SElock);
        lock_release(SElock);
        if( NEcv->count!=0){
            cv_signal(NEcv,NElock);
        }
        message(LEAVING,carnumber,cardirection,cardestination);
        lock_release(NElock);
        
        
    }
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

        (void) cardirection;
        (void) carnumber;
    
        if (cardirection==0){ // Car coming from the north 
		message(APPROACHING, carnumber, cardirection, 3);  //Approaching the intersection from North, going West.           
		lock_acquire(NWlock); //Acquire the lock if its available.
            
            while (NWregion==1){ //Check to see if someone is in the region, if YES, then wait
                cv_wait(NWcv,NWlock); // Go to sleep inside the queue and wait until region is unlocked.
            }

            //IF WE GET HERE -> Region is FREE!
			
			NWregion = 1; 
            
			//proceed to region 1
            message(REGION1,carnumber,cardirection,3);  //Message prints that you are in the region
            
            
            //exit region
            NWregion = 0;
            
            if (NWcv->count!=0) //if there are threads waiting in the queue
                cv_signal(NWcv,NWlock); //allow next thread to wake up
				
			message(LEAVING,carnumber,cardirection,3);	
            lock_release(NWlock); //unlock code

            
        }
        else if (cardirection==1){
            message(APPROACHING, carnumber, cardirection, 0);	
            lock_acquire(NElock);
    	    
            
            while (NEregion==1){
                cv_wait(NEcv,NElock);
            }
			
			
            NEregion = 1;
            
			//proceed to region 1
            message(REGION1,carnumber,cardirection,0);
            
			NEregion=0;
			
            if (NEcv->count !=0){
                cv_signal(NEcv,NElock);
            }
            
			//exit region
            message(LEAVING,carnumber,cardirection,0);
            lock_release(NElock); //unlock code
			
        }
        else if (cardirection==2){
   		message(APPROACHING, carnumber, cardirection, 1);            
	    lock_acquire(SElock);
            
            while (SEregion==1){
                cv_wait(SEcv,SElock);
            }
            
			SEregion = 1;
         
            //proceed to region 1
            message(REGION1,carnumber,cardirection,1);
            
            SEregion = 0;
		   
            if (SEcv->count!=0)
                cv_signal(SEcv,SElock); //allow next thread to wake up
            
			 //exit region
            message(LEAVING,carnumber,cardirection,1);
            lock_release(SElock); //unlock code
        }
		
        else if (cardirection==3){
		message(APPROACHING, carnumber, cardirection, 2);          
	    lock_acquire(SWlock);
             
            while (SWregion==1){
                cv_wait(SWcv,SWlock);
            }
            
            SWregion = 1; 
            //proceed to region 1
            message(REGION1,carnumber,cardirection,2);
           
            //exit region
            SWregion = 0;
			
            
            if (SWcv->count!=0)
                cv_signal(SWcv,SWlock); //allow next thread to wake up
				
			message(LEAVING,carnumber,cardirection,2);	
            lock_release(SWlock); //unlock code
        }
    
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
        int cardirection;
        int cardestination;
		

        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
        (void) gostraight;
        (void) turnleft;
        (void) turnright;
		
		
		lock_acquire(templock);
		while(carlimit>2){
			cv_wait(numcars,templock);
		}
		
        /*
         * cardirection is set randomly. (Where the car is coming from)
         *
         * 0) North
         * 1) East
         * 2) South
         * 3) West
         */
    
        
		carlimit++;
	cardirection = random() % 4;
    
        //* Assigned random direction
        /*  0) Right
         *  1) Left
         *  2) Straight
         */
    
	cardestination = random() %3;
    
    
        lock_release(templock);
    
        //* Indicate to user the intentions of the current car
        //message(APPROACHING, carnumber, cardirection, cardestination)
    
        //* Call the appropriate functions depending on destination
        if (cardestination==0)
            turnright(cardirection,carnumber);
        else if (cardestination==2)
            turnleft(cardirection,carnumber);
        else if (cardestination==1)
            gostraight(cardirection,carnumber);
		
		lock_acquire(templock);
		carlimit = carlimit - 1;
		if (numcars->count!=0)
			cv_signal(numcars,templock);
			
		lock_release(templock);
    
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        /*
         * Avoid unused variable warnings.
         */
    
        (void) nargs;
        (void) args;
        int index, error;
    
    
        /* Create locks for each region
         * NW,NE,SW,SE
         */
        NWlock = lock_create("NWlock");
        SWlock = lock_create("SWlock");
        NElock = lock_create("NElock");
        SElock = lock_create("SElock");
        templock=lock_create("templock");
		numcars = cv_create("numcars");
        NWcv = cv_create("NWcv");
        SWcv = cv_create("SWcv");
        NEcv = cv_create("NEcv");
        SEcv = cv_create("SEcv");
    

        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }

        return 0;
}
