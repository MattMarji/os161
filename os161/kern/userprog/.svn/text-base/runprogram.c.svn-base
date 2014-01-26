/*
 * Sample/test code for running a user program.  You can use this for
 * reference when implementing the execv() system call. Remember though
 * that execv() needs to do more than this function does.
 */

#include <types.h>
#include <kern/unistd.h>
#include <kern/errno.h>
#include <lib.h>
#include <addrspace.h>
#include <thread.h>
#include <curthread.h>
#include <vm.h>
#include <vfs.h>
#include <test.h>

/*
 * Load program "progname" and start running it in usermode.
 * Does not return except on error.
 *
 * Calls vfs_open on progname and thus may destroy it.
 */
 
 /*
int
runprogram(char *progname, char** args, int argv)
{
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;
	int num_args = argv;
	int string_size = 0; //size of each string in char** args.
	size_t check;
	int err;
	
	// Open the file. 
	result = vfs_open(progname, O_RDONLY, &v);
	if (result) {
		return result;
	}

	// We should be a new thread. 
	assert(curthread->t_vmspace == NULL);

	// Create a new address space. 
	curthread->t_vmspace = as_create();
	if (curthread->t_vmspace==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	// Activate it. 
	as_activate(curthread->t_vmspace);

	// Load the executable. 
	result = load_elf(v, &entrypoint);
	if (result) {
		// thread_exit destroys curthread->t_vmspace 
		vfs_close(v);
		return result;
	}

	// Done with the file now. 
	vfs_close(v);

	// Define the user stack in the address space 
	result = as_define_stack(curthread->t_vmspace, &stackptr);
	if (result) {
		// thread_exit destroys curthread->t_vmspace 
		return result;
	}
	
	if (args != NULL){ //arguements exist and we will return to usermode by passing these arguments in!!
		vaddr_t new_args[num_args];
		int padding; // use for alignment if necessary...
		
		//Now check for arguments, and place them on the stack.
	
	int i =0;
	for( i = (num_args-1); i >= 0 ; i-- )
	{
	    
	    string_size = strlen(args[i]);
	    padding = ((string_size / 4 ) + 1)*4;
	   
	    stackptr = stackptr - padding;
	    err = copyoutstr(args[i], stackptr, string_size+1, &check);
	   
	    if(err!=0){
		return err;
	    }
	    new_args[i] = stackptr;
	}

	// Now that we have properly extracted the string we place the pointer to the strings on the stack...
	new_args[num_args] = NULL;
	for(i = num_args; i>=0 ; i--){
	    stackptr= stackptr- 4;
	    err = copyout(&(new_args[i]), stackptr, 4);
	    
		if(err!=0){
		return err;
	    }
	
	
	}
	
	//Return to usermode now.
	md_usermode(num_args, stackptr, stackptr, entrypoint);	
}
	else{ //No arguments!
	
	// Warp to user mode. 
	md_usermode(0 , NULL ,
		    stackptr, entrypoint);
	
	// md_usermode does not return 
	panic("md_usermode returned\n");
	return EINVAL;
	
	}
	
	
}
*/


int
runprogram(char *progname, char** args, int num)
{
    struct vnode *v;
    vaddr_t entrypoint, stackptr;
    int result;
    int s_size = 0;
    int kargc = num;
    int i=0, err;
    size_t tt; 


    /* Open the file. */
    result = vfs_open(progname, O_RDONLY, &v);
    if (result) {
	return result;
    }

    /* We should be a new thread. */
    assert(curthread->t_vmspace == NULL);

    /* Create a new address space. */
    curthread->t_vmspace = as_create();
    if (curthread->t_vmspace==NULL) {
	vfs_close(v);
	return ENOMEM;
    }

    /* Activate it. */
    as_activate(curthread->t_vmspace);

    /* Load the executable. */
    result = load_elf(v, &entrypoint);
    if (result) {
	/* thread_exit destroys curthread->t_vmspace */
	vfs_close(v);
	return result;
    }

    /* Done with the file now. */
    vfs_close(v);

    /* Define the user stack in the address space */
    result = as_define_stack(curthread->t_vmspace, &stackptr);
    if (result) {
	/* thread_exit destroys curthread->t_vmspace */
	return result;
    }

    if(args!=NULL)
    {
	vaddr_t new_args[kargc];
	vaddr_t  new_argv;
	int w_size; /*word alligned size of string*/
     
	/*place all the strings on the stack*/
	/*carefully ignore the first element because contains the program name*/
	for(i = (kargc-1); i>=0 ; i--)
	{
	    
	    s_size = strlen(args[i]);
	    w_size = ((s_size / 4 ) + 1)*4;
	   
	    stackptr -= w_size;
	    err = copyoutstr(args[i], stackptr, s_size+1, &tt);
	   
	    if(err!=0){
		kprintf("2: err %d, the official length is %d, the given length is %d \n ", err, tt, s_size+1);
		return err;
	    }
	    new_args[i] = stackptr;
	}
	kprintf("finished all my strlens \n");

	/*place all the pointers to the strings on the stack*/
	new_args[kargc] = NULL;
	for(i = kargc; i>=0 ; i--){
	    stackptr-= 4;
	    err = copyout(&(new_args[i]), stackptr, 4);
	    if(err!=0){
		kprintf("3: err %d \n ", err);
		return err;
	    }
	}


	/* Warp to user mode. */
	md_usermode(kargc, stackptr, stackptr, entrypoint);
    }
    else
	md_usermode(0, NULL, stackptr, entrypoint);
    /* md_usermode does not return */
    panic("md_usermode returned\n");
    return EINVAL;
}


