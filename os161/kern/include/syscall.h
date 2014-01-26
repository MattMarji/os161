#ifndef _SYSCALL_H_
#define _SYSCALL_H_

/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */
struct trapframe;
 
int sys_reboot(int code);
pid_t sys_fork(struct trapframe *tf,int *ret);
int sys_write(int fd, const void *buf, size_t nbytes);
int sys_read(int fd, const void *buf, size_t buflen);


#endif /* _SYSCALL_H_ */

