/*
 * test02-03-waitexit-5
 * 
 * Test case: 
 *      run: p /testbin/test02-03-waitexit-5
 *      result and error expected: we/testbin/test02-03-waitexit-5: waitpid: Invalid argument 
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

static volatile int mypid;
int pid_p, pid_c;

static int dofork(void)
{
	int pid;
	pid = fork();
	if (pid < 0) {
		warn("fork");
	}
	return pid;
}

/*
 * copied from address space test in forktest
 */
static void check(void)
{
	int i;

	mypid = getpid();
	
	/* Make sure each fork has its own address space. */
	for (i=0; i<800; i++) {
		volatile int seenpid;
		seenpid = mypid;
		if (seenpid != getpid()) {
			errx(1, "pid mismatch (%d, should be %d) "
			     "- your vm is broken!", 
			     seenpid, getpid());
		}
	}
}

/*
 * based on dowait in forktest
 */
static void dowait(int pid)
{

        int x;

	if (pid<0) {
		/* fork in question failed; just return */
		return;
	}
	if (pid==0) {
		/* in the fork in question we were the child; exit */
		exit(0);
	}

	if (waitpid(pid, &x, 0)<0) 
                warn("waitpid");
	else if (x!=0)
                warnx("pid %d: exit %d", pid, x);
		
}

int main()
{

        pid_p = getpid();
        putchar('w');
        pid_c = dofork();


        if (getpid() == pid_p) {
                dowait(pid_c);
        }

        if (getpid() != pid_p) {
                check();
                putchar('e');
                _exit(0);
        } 

        if (getpid() == pid_p) {
                dowait(pid_c);
        } else {
                printf("wrong!\n");
        }

        putchar('\n');

	return 0;
}
