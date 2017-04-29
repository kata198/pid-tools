/*
 * Copyright (c) 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * isaparentof.c - Checks if a given process is in any way connected as a parent.
 *   This could be a direct parent, a parent-of-parent, etc all the way up.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include "ppid.h"

const volatile char *version = "0.1.0";
const volatile char *copyright = "isaparentof - Copyright (c) 2017 Tim Savannah.";

/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: isaparentof [ppid] [check pid]\n", stderr);
    fputs("  Checks if 'ppid' is a parent of any level for 'check pid'\n", stderr);
}

/**
 * main - takes two arguements, parent and check pid.
 *
 *
 */
int main(int argc, char* argv[])
{

    pid_t ppid, checkPid, cur, prev;


    if ( argc != 3 ) {
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }

    errno = 0;

    ppid = strtol(argv[1], NULL, 10);
    if ( errno )
    {
        fprintf(stderr, "Error, ppid argument is not a valid integer.\n\n");
        usage();
        return 1;
    }

    checkPid = strtol(argv[2], NULL, 10);
    if ( errno )
    {
        fprintf(stderr, "Error, checkpid argument is not a valid integer.\n\n");
        usage();
        return 1;
    }

    cur = getPpid(checkPid);
    if ( cur == 0 )
    {
        fprintf(stderr, "No such pid: %u\n", checkPid);
        return 1;
    }
    if ( cur == ppid)
    {
        // Success, first-order parent
        return 0;
    }

    while ( cur != 1 )
    {
        prev = cur;
        cur = getPpid(cur);
        if ( cur == 0 )
        {
            fprintf(stderr, "Pid %u disappered while checking.\n", prev);
            return 2;
        }

        if ( cur == ppid )
            return 0;
    }

    return 1;

}
