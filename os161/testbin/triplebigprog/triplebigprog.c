/*
 * triplebigprog.c
 *
 * 	Calls three triplebigprog programs.
 *
 * When demand paging is complete, your system should survive this.
 */

#include "triple.h"

int
main()
{
	triple("/testbin/bigprog");
	return 0;
}

