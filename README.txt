My current OS161 Implementation.

WHAT HAS BEEN FULLY IMPLEMENTED AND WORKING:

-Threads, Locks, etc.
-Fork
-Execve
-sys_read, sys_write

WHAT HAS BEEN IMPLEMENTED BUT NOT FULLY WORKING:

-Multi-level page table (2-level)

Our OS/161 code has been neatly commented and coded for simple read-through.

All code is in C. This is low-level kernel programming.

The assignments we completed are as follows:

ASST0 (COMPLETED FULLY): http://www.eecg.toronto.edu/~ashvin/courses/ece344/current/asst0.html 
ASST1 (COMPLETED FULLY): http://www.eecg.toronto.edu/~ashvin/courses/ece344/current/asst1.html
ASST2 (COMPLETED 80%, some bugs): http://www.eecg.toronto.edu/~ashvin/courses/ece344/current/asst2.html
ASST3 (COMPLETED 70%, bugs): http://www.eecg.toronto.edu/~ashvin/courses/ece344/current/asst3.html

Note, to compile and run the OS:

cd os161/os161/kern/compile/ASST3 
make depend
make
make install

cd ~/os161/root
sys161 kernel


NOTE: If you would like to use the debugger (GDB) modify the last line to say 'sys161 -w kernel'

Then, in your gdb type: target remote unix:.sockets/gdb 

And you're ready to find bugs!

Thanks to contributions from Heindrik, Karim and Aldrich.

I will likely come back to this code and fix it up in the near future.

-Matt
 