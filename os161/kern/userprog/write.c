//
//  write.c
//  
//
//  Created by Heindrik Bernabe on 2013-09-29.
//
//

#include <kern/errno.h>
#include <kern/unistd.h>
#include <types.h>
#include <lib.h>
#include <syscall.h>




int
sys_write(int fd, const void *buf, size_t nbytes)
{
   char* temp_buffer=kmalloc(sizeof(char)*(nbytes+1));
    
    //copyin function implemented
    copyin((const_userptr_t)buf,temp_buffer,nbytes);
     
     temp_buffer[nbytes]='\0';


    if (buf == NULL){
       // errno = EFAULT;
        return -1;
    }
    else if (temp_buffer == NULL){
       // errno = ENOSPC;
        return -1;
        
    }
    else {
        kprintf("%s",temp_buffer);
    }

	return nbytes;
}
