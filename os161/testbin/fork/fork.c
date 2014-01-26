/*
 * test02-01_fork
 * 
 * Test case: 
 *      run: p /testbin/test02-01-fork
 *      result expected: 00aa111b1bbb2222c22c2cc2cccc
 *
 */

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>

static volatile int mypid;

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

    int pid0, pid1, pid2;

	pid0 = dofork();
	
	putchar('0');
	check();
	putchar('a');

	pid1 = dofork();
	putchar('1');
	check();
	putchar('b');

	pid2 = dofork();
	putchar('2');
	check();
	//putchar('c');

	dowait(pid2);
	dowait(pid1);
	dowait(pid0);

	putchar('\n');

	return 0;
}
