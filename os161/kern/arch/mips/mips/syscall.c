#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <thread.h>
#include <syscall.h>
#include <addrspace.h>
#include <curthread.h>

/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values, 
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	case SYS_reboot:
	err = sys_reboot(tf->tf_a0);
	break;
        
        /* Add stuff here */
        
        case SYS_write:
        err = sys_write(tf->tf_a0,tf->tf_a1,tf->tf_a2);
        break;
		
	case SYS_read:
        err = sys_read(tf->tf_a0,tf->tf_a1,tf->tf_a2);
        break;
				
	case SYS_fork:
	err = sys_fork(tf,&retval);
	break;
		
	case SYS_getpid:
	err = sys_getpid(&retval);
	break;
        
	case SYS_waitpid:
	err = sys_waitpid((pid_t)tf->tf_a0,&retval, tf->tf_a2);
	*((int*)(tf->tf_a1)) = retval;
	break;
	
	case SYS_execv:
	err = sys_execv(tf->tf_a0, tf->tf_a1);
	break;	
	
	case SYS__exit:
	sys_exit(tf->tf_a0);
	break;

	default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}
    
	
	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */
	
	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}


pid_t sys_fork(struct trapframe *tf,pid_t *ret)
{
	pid_t child_pid;
	int result;
	//Allocate heap.
	struct trapframe *child_trapframe = kmalloc(sizeof(struct trapframe));
	//Copy parent trapframe to the to-be child copy.
	memcpy(child_trapframe, tf, sizeof(struct trapframe));
	//Create a new address space.
	struct addrspace *child_addrspace;
	//Copy using as_copy.
	as_copy(curthread->t_vmspace, &child_addrspace);
	struct thread *child_thread;

	result = thread_fork(curthread->t_name, (void*)child_trapframe, (unsigned long)child_addrspace, md_forkentry, &child_thread);
	if (result != 0){
		return result;
	}else//thread_fork returns to PARENT w/ the PID of the child.
	//kprintf("child id: %d ret: %d \n\n\n",child_thread,ret);
	*ret = child_thread->pid;
	return 0;
}
	
//THIS IS THE FIRST FUNCTION THAT THE CHILD WILL CALL WHEN IT IS FORKED!!
void
md_forkentry(void *tf, unsigned long addr)
{

//STEP 1: Copy the trapframe by simply declaring a struct and copying tf there.
	struct trapframe *c_trapframe = (struct trapframe *)tf;
//STEP 2: We make sure the child thread returns to usermode but ensuring we set the v0 and a0 registers to 0 and incrementing the PC counter by 4 to state that we successfully finished.

	(c_trapframe)->tf_v0 = 0;
	(c_trapframe)->tf_a3 = 0;
	(c_trapframe)->tf_epc += 4; 	
	
	struct trapframe stack_trapframe;
	memcpy(&stack_trapframe,c_trapframe, sizeof(struct trapframe));
	
	c_trapframe = &stack_trapframe;
	
	//STEP 3: We need to take the copy of the address space of the parent, and copy it to the child's 
	curthread->t_vmspace = (struct addrspace *)addr;

	as_activate(curthread->t_vmspace);
	
	mips_usermode(c_trapframe); //Trapframe of child thread sent in.
}

