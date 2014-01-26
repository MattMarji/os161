/*
 * test02-03-waitexit-4
 * 
 * Test case: 
 *      run: p /testbin/test02-03-waitexit-4
 *      result expected: att
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

int main()
{

        pid_p = getpid();
        putchar('a');
        pid_c = dofork();

        if (getpid() == pid_p) {
                check();
                putchar('t');
                _exit(0);
        } else {
                putchar('t');
                _exit(0);
        }

        putchar('e');
        putchar('\n');

        return 0;
}
