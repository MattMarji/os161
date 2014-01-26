/*
 * Core thread system.
 */
#include <types.h>
#include <kern/unistd.h>
#include <lib.h>
#include <kern/errno.h>
#include <array.h>
#include <machine/spl.h>
#include <machine/pcb.h>
#include <thread.h>
#include <curthread.h>
#include <scheduler.h>
#include <addrspace.h>
#include <vnode.h>
#include "opt-synchprobs.h"
#include <synch.h>
#include <vfs.h>
#include <test.h>

// ASST 2
struct cv *cv_parent_queue;
struct lock *lock_thread_join;
struct lock *lock_addpid;
struct lock *lock_exit;




/* States a thread can be in. */
typedef enum {
	S_RUN,
	S_READY,
	S_SLEEP,
	S_ZOMB,
} threadstate_t;

/* Global variable for the thread currently executing at any given time. */
struct thread *curthread;

/* Table of sleeping threads. */
static struct array *sleepers;

/* List of dead threads to be disposed of. */
static struct array *zombies;

/* Total number of outstanding threads. Does not count zombies[]. */
static int numthreads;

/* Create a global head pointer for our PID linked list */
struct pid_node *pid_head = NULL;

/* Create a global lock to be used for atomic work. */
struct lock *lock;
struct lock *wait_lock; //use just in waitpid


/*
 * Create a thread. This is used both to create the first thread's 
 * thread structure and to create subsequent threads.
 */

static
struct thread *
thread_create(const char *name)
{
	struct thread *thread = kmalloc(sizeof(struct thread));
	if (thread==NULL) {
		return NULL;
	}
	thread->t_name = kstrdup(name);
	if (thread->t_name==NULL) {
		kfree(thread);
		return NULL;
	}
	thread->t_sleepaddr = NULL;
	thread->t_stack = NULL;
	thread->t_vmspace = NULL;
	thread->t_cwd = NULL;
	
	// If you add things to the thread structure, be sure to initialize
	// them here.
	// 8=================================================================D
	
	//Assume that the thread does not have a parent. If it remains 0 it is true, otherwise, it has a parent.
	thread->parent_pid=0;
	
	//Create a PID for the thread.
	lock_acquire(lock_addpid);
	thread->pid = add_pid(); //assign thread a pid
	//kprintf("CURTHREAD PID IS %d \n", thread->pid);
	lock_release(lock_addpid);
	if (thread->pid == -1){
		return NULL;
	}
	
	return thread;
}

/*
 * Destroy a thread.
 *
 * This function cannot be called in the victim thread's own context.
 * Freeing the stack you're actually using to run would be... inadvisable.
 */
static
void
thread_destroy(struct thread *thread)
{
	assert(thread != curthread);

	// If you add things to the thread structure, be sure to dispose of
	// them here or in.

	// These things are cleaned up in thread_exit.
	assert(thread->t_vmspace==NULL);
	assert(thread->t_cwd==NULL);
	
	if (thread->t_stack) {
		kfree(thread->t_stack);
	}

	kfree(thread->t_name);
	kfree(thread);
}


/*
 * Remove zombies. (Zombies are threads/processes that have exited but not
 * been fully deleted yet.)
 */
static
void
exorcise(void)
{
	int i, result;

	assert(curspl>0);
	
	for (i=0; i<array_getnum(zombies); i++) {
		struct thread *z = array_getguy(zombies, i);
		assert(z!=curthread);
		thread_destroy(z);
	}
	result = array_setsize(zombies, 0);
	/* Shrinking the array; not supposed to be able to fail. */
	assert(result==0);
}

/*
 * Kill all sleeping threads. This is used during panic shutdown to make 
 * sure they don't wake up again and interfere with the panic.
 */
static
void
thread_killall(void)
{
	int i, result;
	assert(curspl>0);

	/*
	 * Move all sleepers to the zombie list, to be sure they don't
	 * wake up while we're shutting down.
	 */

	for (i=0; i<array_getnum(sleepers); i++) {
		struct thread *t = array_getguy(sleepers, i);
		kprintf("sleep: Dropping thread %s\n", t->t_name);

		/*
		 * Don't do this: because these threads haven't
		 * been through thread_exit, thread_destroy will
		 * get upset. Just drop the threads on the floor,
		 * which is safer anyway during panic.
		 *
		 * array_add(zombies, t);
		 */
	}

	result = array_setsize(sleepers, 0);
	/* shrinking array: not supposed to fail */
	assert(result==0);
}

/*
 * Shut down the other threads in the thread system when a panic occurs.
 */
void
thread_panic(void)
{
	assert(curspl > 0);

	thread_killall();
	scheduler_killall();
}

/*
 * Thread initialization.
 */
struct thread *
thread_bootstrap(void)
{
	struct thread 
	lock_exit = lock_create("lock_exit");
	cv_parent_queue = cv_create("parent_queue");

	/* Create the DAta structures we need. */
	sleepers = array_create();
	if (sleepers==NULL) {
		panic("Cannot create sleepers array\n");
	}

	zombies = array_create();
	if (zombies==NULL) {
		panic("Cannot create zombies array\n");
	}
	
	/*
	 * Create the thread structure for the first thread
	 * (the one that's already running)
	 */
	me = thread_create("<boot/menu>");
	if (me==NULL) {
		panic("thread_bootstrap: Out of memory\n");
	}

	/*
	 * Leave me->t_stack NULL. This means we're using the boot stack,
	 * which can't be freed.
	 */

	/* Initialize the first thread's pcb */
	md_initpcb0(&me->t_pcb);

	/* Set curthread */
	curthread = me;

	/* Number of threads starts at 1 */
	numthreads = 1;

	/* Done */
	return me;
}

/*
 * Thread final cleanup.
 */
void
thread_shutdown(void)
{
	array_destroy(sleepers);
	sleepers = NULL;
	array_destroy(zombies);
	zombies = NULL;
	// Don't do this - it frees our stack and we blow up
	//thread_destroy(curthread);
}

/*
 * Create a new thread based on an existing one.
 * The new threaD has name NAME, and starts executing in function FUNC.
 * DATA1 and DATA2 are passed to FUNC.
 */
int
thread_fork(const char *name, 
	    void *data1, unsigned long data2,
	    void (*func)(void *, unsigned long),
	    struct thread **ret)
{
	
	struct thread *newguy;
	int s, result;

	/* Allocate a thread */
	newguy = thread_create(name);
	if (newguy==NULL) {
		return ENOMEM;
	}

	/* Allocate a stack */
	newguy->t_stack = kmalloc(STACK_SIZE);
	if (newguy->t_stack==NULL) {
		kfree(newguy->t_name);
		kfree(newguy);
		return ENOMEM;
	}

	/* stick a magic number on the bottom end of the stack */
	newguy->t_stack[0] = 0xae;
	newguy->t_stack[1] = 0x11;
	newguy->t_stack[2] = 0xda;
	newguy->t_stack[3] = 0x33;

	/* Inherit the current directory */
	if (curthread->t_cwd != NULL) {
		VOP_INCREF(curthread->t_cwd);
		newguy->t_cwd = curthread->t_cwd;
	}
	
	/* Set up the pcb (this arranges for func to be called) */
	md_initpcb(&newguy->t_pcb, newguy->t_stack, data1, data2, func);

	/* Interrupts off for atomicity */
	s = splhigh();
	
	/*
	 * Make sure our data structures have enough space, so we won't
	 * run out later at an inconvenient time.
	 */
	result = array_preallocate(sleepers, numthreads+1);
	if (result) {
		goto fail;
	}
	result = array_preallocate(zombies, numthreads+1);
	if (result) {
		goto fail;
	}

	/* Do the same for the scheduler. */
	result = scheduler_preallocate(numthreads+1);
	if (result) {
		goto fail;
	}

	/* Make the new thread runnable */
	result = make_runnable(newguy);
	if (result != 0) {
		goto fail;
	}

	/*
	 * Increment the thread counter. This must be done atomically
	 * with the preallocate calls; otherwise the count can be
	 * temporarily too low, which would obviate its reason for
	 * existence.
	 */
	numthreads++;

	//The child's Parent PID (curthread->pid) is stored in its PPID
	newguy->parent_pid = curthread->pid;


	struct pid_node *temp;
	struct pid_node *temp2;	
	
	temp = find_node(curthread->pid); //return the pid_node of the parent.
		
	temp2 = find_node(newguy->pid); //return the pid_node of the forked child.

	if (temp->next_children == NULL) //this means that the parent has no children.
	{
		temp->next_children = temp2;
	}	
	else{ //HAS CHILDREN.
		temp = temp->next_children;
		while (temp->next_siblings != NULL){
			temp = temp->next_siblings;
		}
		temp->next_siblings = temp2;		
	
	}	
	

	/* Done with stuff that needs to be atomic */
	splx(s);

	/*
	 * Return new thread structure if it's wanted.  Note that
	 * using the thread structure from the parent thread should be
	 * done only with caution, because in general the child thread
	 * might exit at any time.
	 */
	 
	
	
	if (ret != NULL) {
		*ret = newguy;
	}

	return 0;
	

 fail:
	splx(s);
	if (newguy->t_cwd != NULL) {
		VOP_DECREF(newguy->t_cwd);
	}
	kfree(newguy->t_stack);
	kfree(newguy->t_name);
	kfree(newguy);

	return result;
}

/*
 * High level, machine-independent context switch code.
 */
static
void
mi_switch(threadstate_t nextstate)
{
	struct thread *cur, *next;
	int result;
	
	/* Interrupts should already be off. */
	assert(curspl>0);

	if (curthread != NULL && curthread->t_stack != NULL) {
		/*
		 * Check the magic number we put on the bottom end of
		 * the stack in thread_fork. If these assertions go
		 * off, it most likely means you overflowed your stack
		 * at some point, which can cause all kinds of
		 * mysterious other things to happen.
		 */
		assert(curthread->t_stack[0] == (char)0xae);
		assert(curthread->t_stack[1] == (char)0x11);
		assert(curthread->t_stack[2] == (char)0xda);
		assert(curthread->t_stack[3] == (char)0x33);
	}
	
	/* 
	 * We set curthread to NULL while the scheduler is running, to
	 * make sure we don't call it recursively (this could happen
	 * otherwise, if we get a timer interrupt in the idle loop.)
	 */
	if (curthread == NULL) {
		return;
	}
	cur = curthread;
	curthread = NULL;

	/*
	 * Stash the current thread on whatever list it's supposed to go on.
	 * Because we preallocate during thread_fork, this should not fail.
	 */

	if (nextstate==S_READY) {
		result = make_runnable(cur);
	}
	else if (nextstate==S_SLEEP) {
		/*
		 * Because we preallocate sleepers[] during thread_fork,
		 * this should never fail.
		 */
		result = array_add(sleepers, cur);
	}
	else {
		assert(nextstate==S_ZOMB);
		result = array_add(zombies, cur);
	}
	assert(result==0);

	/*
	 * Call the scheduler (must come *after* the array_adds)
	 */

	next = scheduler();

	/* update curthread */
	curthread = next;
	
	/* 
	 * Call the machine-dependent code that actually does the
	 * context switch.
	 */
	md_switch(&cur->t_pcb, &next->t_pcb);
	
	/*
	 * If we switch to a new thread, we don't come here, so anything
	 * done here must be in mi_threadstart() as well, or be skippable,
	 * or not apply to new threads.
	 *
	 * exorcise is skippable; as_activate is done in mi_threadstart.
	 */

	exorcise();

	if (curthread->t_vmspace) {
		as_activate(curthread->t_vmspace);
	}
}

/*
 * Cause the current thread to exit.
 *
 * We clean up the parts of the thread structure we don't actually
 * need to run right away. The rest has to wait until thread_destroy
 * gets called from exorcise().
 */
void
thread_exit(void)
{
	if (curthread->t_stack != NULL) {
		/*
		 * Check the magic number we put on the bottom end of
		 * the stack in thread_fork. If these assertions go
		 * off, it most likely means you overflowed your stack
		 * at some point, which can cause all kinds of
		 * mysterious other things to happen.
		 */
		assert(curthread->t_stack[0] == (char)0xae);
		assert(curthread->t_stack[1] == (char)0x11);
		assert(curthread->t_stack[2] == (char)0xda);
		assert(curthread->t_stack[3] == (char)0x33);
	}

	splhigh();

	if (curthread->t_vmspace) {
		/*
		 * Do this carefully to avoid race condition with
		 * context switch code.
		 */
		struct addrspace *as = curthread->t_vmspace;
		curthread->t_vmspace = NULL;
		as_destroy(as);
	}

	if (curthread->t_cwd) {
		VOP_DECREF(curthread->t_cwd);
		curthread->t_cwd = NULL;
	}

	assert(numthreads>0);
	numthreads--;
	mi_switch(S_ZOMB);

	panic("Thread came back from the dead!\n");
}

/*
 * Yield the cpu to another process, but stay runnable.
 */
void
thread_yield(void)
{
	int spl = splhigh();

	/* Check sleepers just in case we get here after shutdown */
	assert(sleepers != NULL);

	mi_switch(S_READY);
	splx(spl);
}

/*
 * Yield the cpu to another process, and go to sleep, on "sleep
 * address" ADDR. Subsequent calls to thread_wakeup with the same
 * value of ADDR will make the thread runnable again. The address is
 * not interpreted. Typically it's the address of a synchronization
 * primitive or data structure.
 *
 * Note that (1) interrupts must be off (if they aren't, you can
 * end up sleeping forever), and (2) you cannot sleep in an 
 * interrupt handler.
 */
void
thread_sleep(const void *addr)
{
	// may not sleep in an interrupt handler
	assert(in_interrupt==0);
	
	curthread->t_sleepaddr = addr;
	mi_switch(S_SLEEP);
	curthread->t_sleepaddr = NULL;
}

/*
 * Wake up one or more threads who are sleeping on "sleep address"
 * ADDR.
 */
void
thread_wakeup(const void *addr)
{
	int i, result;
	
	// meant to be called with interrupts off
	assert(curspl>0);
	
	// This is inefficient. Feel free to improve it.
	
	for (i=0; i<array_getnum(sleepers); i++) {
		struct thread *t = array_getguy(sleepers, i);
		if (t->t_sleepaddr == addr) {
			
			// Remove from list
			array_remove(sleepers, i);
			
			// must look at the same sleepers[i] again
			i--;

			/*
			 * Because we preallocate during thread_fork,
			 * this should never fail.
			 */
			result = make_runnable(t);
			assert(result==0);
		}
	}
}

/*
 * Return nonzero if there are any threads who are sleeping on "sleep address"
 * ADDR. This is meant to be used only for diagnostic purposes.
 */
int
thread_hassleepers(const void *addr)
{
	int i;
	
	// meant to be called with interrupts off
	assert(curspl>0);
	
	for (i=0; i<array_getnum(sleepers); i++) {
		struct thread *t = array_getguy(sleepers, i);
		if (t->t_sleepaddr == addr) {
			return 1;
		}
	}
	return 0;
}

/*
 * New threads actually come through here on the way to the function
 * they're supposed to start in. This is so when that function exits,
 * thread_exit() can be called automatically.
 */
void
mi_threadstart(void *data1, unsigned long data2, 
	       void (*func)(void *, unsigned long))
{
	/* If we have an address space, activate it */
	if (curthread->t_vmspace) {
		as_activate(curthread->t_vmspace);
	}

	/* Enable interrupts */
	spl0();

#if OPT_SYNCHPROBS
	/* Yield a random number of times to get a good mix of threads */
	{
		int i, n;
		n = random()%161 + random()%161;
		for (i=0; i<n; i++) {
			thread_yield();
		}
	}
#endif
	
	/* Call the function */
	func(data1, data2);

	/* Done. */
	thread_exit();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////  PID CODE FOR OUR LINKED LIST			////////////////////
////////////////////										////////////////////
////////////////////////////////////////////////////////////////////////////////
//We use a PID linked list to keep track and assign PIDS. When a PID is removed, the next PID will take its spot.

//Allocate memory and initialize a node of type pid_node. Return this node.
struct pid_node* create_node(){
	struct pid_node *node;
	node = kmalloc(sizeof(struct pid_node));
	node->exited = 0; //originally set to 0 meaning the thread has not yet exited.s
	node->pid=0;
	node->next=NULL;
	node->next_children = NULL;
	node->next_siblings = NULL;
	return node;

}

//find the node associated with my pid
struct pid_node* find_node(pid_t pid){
	if (pid_head==NULL){
		//there is nothing made
		return -1;
	}
	struct pid_node* curr;
	curr = pid_head;
	//find the node and return it
	while(curr!=NULL){
		if (curr->pid != pid){
			curr = curr->next;
		}else if (curr->pid == pid){
			return curr;
		}
	}
	return -1; // doesn't exist
}

int find_exitcode(pid_t pid){
	if (pid_head == NULL){
	 return NULL;
	}
	struct pid_node *curr;
	curr = pid_head;
	while (curr->next != NULL){
		if (curr->pid == pid){ 
				return curr->exited;
		}
		curr = curr->next;
	}
	// pid does not exist
	// used just as a precaution
	return NULL;
}

//Create a linked list of pids!

pid_t add_pid(){
	//lock_acquire(lock_addpid);
	
	//Check if there is a linked list. 
	if (pid_head == NULL){
		//no pid exists, create one.
		pid_head = create_node();
			if (pid_head == NULL) //mem alloc fail
				return -1;
		pid_head->my_thread = curthread;
		pid_head->pid = 1; //Note sure what the min value is -- check.
		pid_head->next = NULL;
		kprintf("DEBUG: We are creating the first node \n");
		return pid_head->pid;
		}
	struct pid_node *node;
	struct pid_node *curr;
	curr = pid_head;

	while (curr != NULL){
	 
	    //kprintf("current pid - %d , previos pid - %d \n", curr->pid, prev->pid);
		//check to see where to place the next pid.
		
		if(curr->next == NULL){
			node = create_node();
				if (node == NULL){
				return -1;
				}
			node->my_thread = curthread;
			node->pid = curr->pid+1;
			curr->next = node;			
			node->next=NULL;
			return node->pid;
		}
		curr = curr->next;
	}
	
	kprintf("DEBUG: add_pid function - while loop broke \n");
}


/*
//Create a linked list of pids.
pid_t add_pid(){
	lock_acquire(lock_addpid);
	//Check if there is a linked list. 
	if (pid_head == NULL){
		//no pid exists, create one.
		pid_head = create_node();
		if (pid_head == NULL){ //mem alloc fail
			lock_release(lock_addpid);				
			return -1;//ENOMEM;
		}
		pid_head->pid = 1; //Note sure what the min value is -- check.
		pid_head->next = NULL;
		lock_release(lock_addpid);
		return pid_head->pid;
		}
	struct pid_node *node;
	struct pid_node *curr, *prev;
	prev = pid_head;
	curr = pid_head->next;

	while (curr != NULL){
		//check to see where to place the next pid.
		
		if(prev->pid == curr->pid)
			break; 
			
		
		if ((curr->pid - prev->pid) > 1) //there is a gap between the two pids.
		{
			node = create_node();
			node->pid = prev->pid+1;
			node->next = curr;
			prev->next = node;
			lock_release(lock_addpid);
			return node->pid;
		}
		prev = curr;
		curr = curr->next;
	}

	//If we jump out here, then there are no spots available inbetween existing PIDs.
	//We must add the PID to the end.

	if (prev->pid > 4096) //Max PID Value
	{
		lock_release(lock_addpid);
		return -1;//EINVAL;
	}
	
	node = create_node();
	node->pid = prev->pid+1;
	node->next=prev->next;
	prev->next = node;
	lock_release(lock_addpid);
	return node->pid;

}
*/

//FIND IF  A PID EXISTS, AND IF IT HAS EXITED OR NOT IF IT EXISTS.
//FAIL: -1 == PID D.N.E
//FAIL: 0  == PID EXISTS, HAS NOT EXITED.
//SUCCESS: 1 == PID EXISTED, HAS EXITED.

int find_pid(pid_t pid){
	if (pid_head == NULL){
	 return -1;
	}
	struct pid_node *curr;
	curr = pid_head;
	while (curr->next != NULL){
	
		if (curr->pid == pid){
			if (curr->exited == 0){ //not exited
			return 0;}
			else{ 
			return curr->exitcode;}
		}
		
		curr = curr->next;
	}
	
	//If we come out here, then invalid PID
	return -1;
}


//IF a thread with a matching PID exists, and it has NOT YET exited, return the matching thread struct!
//FAIL: RETURN NULL.
//SUCCESS: RETURN POINTER TO MATCHING THREAD.
struct thread* return_thread_pid(pid_t pid){
	if (pid_head == NULL){
	 return NULL;
	}
	struct pid_node *curr;
	curr = pid_head;
	while (curr->next != NULL){
	
		if (curr->pid == pid){
			if (curr->exited == 0){ //not exited
			return NULL;}
			else{ 
			return curr->my_thread;}
		}
		
		curr = curr->next;
	}
	
	//If we come out here, then invalid PID
	return NULL;
}



int remove_pid (pid_t pid){

if (pid_head == NULL)
	return -1; //no linked list exists -- ERROR CODE?
	
	struct pid_node *curr, *prev;
	prev = pid_head;
	curr = prev->next;

	if (curr == NULL){
	pid_head = NULL;
	kfree(prev);
	return 0;
	}

	while (curr != NULL){
	
		if (curr->pid == pid){
			if (prev == NULL){
				pid_head = curr->next;
			}
			else{
				prev->next = curr->next;
			}
			kfree(curr);
			return 0;
		}
		
		if (curr->pid > pid){
		return -1;
		}
	
		prev = curr;
		curr = curr->next;
	}

	if (curr == NULL)
		return EINVAL; //no matching pid.


	return 0;
}


int sys_getpid(int *ret){
	*ret = curthread->pid;
	return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////    WAITPID CREATED HERE     /////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int sys_waitpid(pid_t pid, int *status, int options){
	
	
	//LOCK ACQUIRE -- prevent race conditions
	lock_acquire(wait_lock);
	int check;
	int stat;
	//check = copyin((userptr_t)status, &stat,sizeof(status)); //copy the status content from user to kernel.
	
	//STEP 1: Check validity of arguments
		
		//Check if the options field is set to 0
		if (options != 0){
			lock_release(wait_lock);
			kprintf("options fucked \n");
			return EINVAL; //ERROR
		}
		
		//Checks for alignment, and other issues that would cause copyin to fail.
		/*if (check != 0){
			lock_release(wait_lock);
			return -2; //EFAULT ERROR
		}*/
		
		//Check if the status pointer given is NULL
		if (status == NULL){
			kprintf("\n\n status failed \n\n");
			lock_release(wait_lock);
			return EFAULT;
		}
;
	//STEP 2: Check validity of PID givent
		struct pid_node *temp1;
		//did the child exit already?
		temp1 = find_node(curthread->pid); 
		if (temp1->next_children==NULL){
			//I don't even have a child
			lock_release(wait_lock);
			kprintf("child does not exist \n");			
			return -1;
		}else{
			temp1=temp1->next_children;
			while(temp1->next_siblings!=NULL){
				if (temp1->pid==pid){
					break;
				}
				temp1=temp1->next_siblings;
			}
			if (temp1->pid!=pid){ //remember to check the last sibling in the list
				lock_release(wait_lock);	
				kprintf("check last sibling \n");		   	
				return -1;
			}
			else if(temp1->exited==1 && temp1->pid == pid){ //child already exited
				// return the exitcode value	
				//check=copyout(&temp1->exitcode, status, sizeof(int)); //86% confident
				*status = temp1->exitcode;
				/*if (check != 0){
					kprintf("\n\n copyout check \n\n");
					lock_release(wait_lock);
					return -2; //EFAULT ERROR
				}*/
				lock_release(wait_lock);
				//kprintf("child already exited - return exitcode \n");
				return 0;
			}
		}//if we make it here then we have found the PID and its valid
	
	
	//STEP 3: Call thread_join() - holding wait_lock still!
		thread_join(pid); 
		
		//return from thread join here. Child has exited.		
		temp1 = find_node(pid);
		*status = temp1->exitcode;
		//set the exit code
		//check=copyout(&temp1->exitcode, status, sizeof(int)); //86.36% confident
		/*if (check != 0){
			kprintf("\n\n copyout check2 \n\n");
			lock_release(wait_lock);
			return -2; //EFAULT ERROR
		}*/

	//IF we step out here, then there are no errors, child thread exists and has not yet exited.
		lock_release(wait_lock);
			return 0;
	
}


//THREAD JOIN FUNCTION - only used with waitpid
int thread_join(pid_t pid){
		
	//function has confidence that the pid does exist.	
 	struct pid_node *temp10;
	temp10 = find_node(pid); //this is pointing to the child
	while(temp10->exited==0){	
		cv_wait(cv_parent_queue,wait_lock);
	}
	// parent is done waiting, and now goes back
	
	return 0; 
}

//EXIT FUNCTION
int sys_exit(int exitcode){
	lock_acquire(lock_exit);
	struct pid_node *temp12;
	temp12 = find_node(curthread->pid);
	temp12->exited=1;
	temp12->exitcode=exitcode;
	if((cv_parent_queue->count)>0)
		cv_broadcast(cv_parent_queue,lock_exit);
	lock_release(lock_exit);
	thread_exit();
}

int sys_execv(char* progname, char** args){

    struct vnode *v;
    vaddr_t stackptr, entrypoint;
    int result;
    int string_size = 0; //s_size
    int kargc = 0; //kargc
    int err;
    char ** kernel_args; //kargs
    size_t check; //tt
    
	//CHECKS BEFORE WE BEGIN!!
    if(args == NULL)
		return EFAULT;
		
    if(progname == NULL)
		return ENOEXEC;
    
    struct addrspace* temp =(struct addrspace*)kmalloc(sizeof(struct addrspace));
		if(temp == NULL)
			return ENOMEM;
    
		while(args[kargc] != NULL){
			kargc++;
	   }

	   //Copy the address space to the one on the kernel heap.
		err = as_copy(curthread->t_vmspace, &(temp));
		
		//If there is an error on copy.
		if(err){
		kfree(temp);
		return ENOMEM;
		}
   
   //make a copy of the kernel args on the heap.
   kernel_args = (char **)kmalloc((kargc)*sizeof(char*));
		if(kernel_args == NULL){
		 kfree(kernel_args);
		return ENOMEM;
		}
   int i,j;
	//Transfer the items passed in to the kernel heap array.
    for(i = 0; i<kargc; i++){
	string_size = strlen(args[i]);
	kernel_args[i] =(char *)kmalloc(string_size * sizeof(char));
	
	//If there is not enough memory...
	if(kernel_args[i]== NULL){
	    kfree(temp);
	    for(j =0; j<i; j++){
		kfree(kernel_args[j]);
		}
	    kfree(kernel_args);
	    return ENOMEM;
	}
	
	//If we get here, we have successfully copied the arguments. Copy in now.
	
	err = copyinstr(args[i], kernel_args[i], string_size+1, &check);
	
	//Check if copyinstr Success/Fail
	if(err){
	
	    for(i = 0; i<kargc; i++){
		kfree(kernel_args[i]);
		}
	    kfree(temp);
		kfree(kernel_args);
	    return err;
		}
    }
	
    kernel_args[kargc]= NULL;

	//NOW, WE FOLLOW THE SAME PROCEDURE AS RUNPROGRAM!!


/* Open the file. */
    result = vfs_open(progname, O_RDONLY, &v);
    if (result) {
	 kfree(temp);
	for(i = 0; i<kargc; i++)
	    kfree(kernel_args[i]);
		kfree(kernel_args);
	return result;
    }

//Destroy the old address space, and setup the new one!
	
    as_destroy(curthread->t_vmspace);
    curthread->t_vmspace = NULL;

    //Setup new vmspace.
    curthread->t_vmspace = as_create();
	
    if (curthread->t_vmspace==NULL) {
	curthread->t_vmspace = temp;
	
	for(i = 0; i<kargc; i++)
	    kfree(kernel_args[i]);
		kfree(kernel_args);
		vfs_close(v);
		return ENOMEM;
    }

    /* Activate vmspace */
    as_activate(curthread->t_vmspace);

    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
	as_destroy(curthread->t_vmspace);
	curthread->t_vmspace = temp;
	
	 
	for(i = 0; i<kargc; i++)
	    kfree(kernel_args[i]);
	kfree(kernel_args);
	vfs_close(v);
	return result;
    }

    /* Done with the file now. */
    vfs_close(v);

   //We now prepare the user stack by placing the arguments on it, like we did in runprogram.
   
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    
	//If error:
	if (result) {
	as_destroy(curthread->t_vmspace);
	curthread->t_vmspace = temp;

	for(i = 0; i<kargc; i++)
	    kfree(kernel_args[i]);
		kfree(kernel_args);
	return result;
    }
	// new addr space user stack
    vaddr_t new_args[kargc];
    int padding; 
     
    /*place all the strings on the stack*/
    
    for(i = (kargc-1); i>=0 ; i--){
	string_size = strlen(kernel_args[i]);
	padding = ((string_size / 4 ) + 1)*4;
	
	stackptr = stackptr - padding;
	
	//Do a copyout and check if success/fail
	err = copyoutstr(kernel_args[i], stackptr, string_size+1, &check);
		if(err){
			return err;
		}
	
		//Success!
		new_args[i] = stackptr;
    }

  
    new_args[kargc] = NULL;
    for(i = kargc-1; i>=0 ; i--){
	stackptr= stackptr- 4;
	err = copyout(&(new_args[i]), stackptr, 4);
		if(err){
			return err;
		}
   }
    for(i =0; i<kargc; i++){
		kfree(kernel_args[i]);
	}
    kfree(temp);
	kfree(kernel_args);
        
    /* Warp to user mode. */
    md_usermode(kargc, stackptr, stackptr, entrypoint);
	

}
