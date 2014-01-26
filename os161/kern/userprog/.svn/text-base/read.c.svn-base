//
//  read.c
//  Attempt to read the bytes in the buffer in the terminal. Return errors on failure.
//
//  Created by MARJIIIIII on 2013-10-19.
//
//

#include <kern/errno.h>
#include <kern/unistd.h>
#include <types.h>
#include <lib.h>
#include <syscall.h>
#include <thread.h>

//Assuming that the necessary input is in *buf, we will return an integer of size buflen.

int sys_read(int fd, const void *buf, size_t buflen){
 int result;
 
 if (!buflen)
	return 0;
	
//Create a temporary array in the kernel to retrieve the STDIN.
char* temp_buffer=kmalloc((buflen)*sizeof(char));

//Retrieve the STDIN  
int i;
    for(i = 0;i<buflen; i++ ){
	temp_buffer[i] = getch();
    }
    temp_buffer[buflen] = NULL;


//copy stored input to the user buffer.
result = copyout(temp_buffer,buf,sizeof(buf));

	if (result)
		return result;
		
 return buflen;
}
