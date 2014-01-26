/* bigprog.c
 * Operate on a big array. The array is huge but we only operate on a part of
 * it. This is designed to test demand paging, when swapping is not implemented
 * yet */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* 1 MB array */
#define SIZE ((1024 * 1024)/sizeof(u_int32_t))
u_int32_t bigarray[SIZE];

/* access 64 KB of the array */
#define MAX ((64 * 1024)/sizeof(u_int32_t))

int
main()
{
        u_int32_t i;

        for (i = 0; i < MAX; i++) {
                bigarray[i] = i;
        }
        for (i = 0; i < MAX; i++) {
                if (bigarray[i] != i) {
                        printf("bigprog test failed\n");
                        exit(1);
                }
        }
        printf("Passed bigprog test.\n");
        exit(0);
}
