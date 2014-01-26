/*
 * this program multiplies two numbers.
 * one is a global variable, the other is local variable.
 * both variables are assigned from the arguments of main. 
 * Used to test argument passing to child processes to test the execv() system
 * call implementation. 
 *
 * Test case:
 *      run: p /testbin/multiply 3 5
 *      result expected: 15
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>

int mult1 = 0;

int
main(int argc, char *argv[])
{
	int mult2;

        if (argc != 3)
                return -1;

        mult1 = atoi(argv[1]);
	mult2 = atoi(argv[2]);

	printf("%d\n", mult1 * mult2);

	return 0;
}
