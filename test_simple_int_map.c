/*
 * Copyright (c) 2018 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * test_simple_int_map.c - Test program for the simple int map
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pid_tools.h"
#include "simple_int_map.h"

int main(int argc, char* argv[])
{
    SimpleIntMap *intMap;
    int ret;
    int *values;
    size_t valuesSize;
    int i;

    intMap = simple_int_map_create(10);

    ret = simple_int_map_add(intMap, 1400); printf("1400 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 1400); printf("1400 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 46); printf("46 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 13906); printf("13906 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 1401); printf("1401 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 1400); printf("1400 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 105); printf("105 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 1005); printf("1005 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 10005); printf("10005 Ret=%d\n", ret);

    printf("Doing contins..\n");

    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_contains(intMap, 100005); printf("contains 100005? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_contains(intMap, 371); printf("contains 371? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_contains(intMap, 1400); printf("contains 1400? %s\n", ret == 1 ? "true" : "false");

    printf("Trying removes\n");
    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
    printf("Removing 1005 ");
    ret = simple_int_map_rem(intMap, 1005);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
    printf("Removing 1005 ");
    ret = simple_int_map_rem(intMap, 1005);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    printf("Adding 1005.\n");
    ret = simple_int_map_add(intMap, 1005); printf("1005 Ret=%d\n", ret);
    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
    printf("Removing 1005 ");
    ret = simple_int_map_rem(intMap, 1005);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
    printf("Removing 105 ");
    ret = simple_int_map_rem(intMap, 105);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 105); printf("contains 105? %s\n", ret == 1 ? "true" : "false");
    printf("Removing 105 ");
    ret = simple_int_map_rem(intMap, 105);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 105); printf("contains 105? %s\n", ret == 1 ? "true" : "false");
    printf("Removing 10005 ");
    ret = simple_int_map_rem(intMap, 10005);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 10005); printf("contains 10005? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_contains(intMap, 105); printf("contains 105? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
    printf("Adding back\n");
    ret = simple_int_map_add(intMap, 105); printf("105 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 1005); printf("1005 Ret=%d\n", ret);
    ret = simple_int_map_add(intMap, 10005); printf("10005 Ret=%d\n", ret);
    ret = simple_int_map_contains(intMap, 10005); printf("contains 10005? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_contains(intMap, 105); printf("contains 105? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_rem(intMap, 105);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 105); printf("contains 105? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_rem(intMap, 10005);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 10005); printf("contains 10005? %s\n", ret == 1 ? "true" : "false");
    ret = simple_int_map_rem(intMap, 1005);
    printf("success = %s\n", ret == 1 ? "true" : "false" );
    ret = simple_int_map_contains(intMap, 1005); printf("contains 1005? %s\n", ret == 1 ? "true" : "false");
 


    values = simple_int_map_values(intMap, &valuesSize);

    printf("Got %ld values\n", valuesSize);

    for(i=0; i<valuesSize; i++)
    {
        printf("values[%d] = %d\n", i, values[i]);
    }

    free(values);
    simple_int_map_destroy(intMap);

    return 0;
}

