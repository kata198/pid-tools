/*
 * Copyright (c) 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * isachildof.c - Checks if a given process is in any way connected as a child.
 *   This could be a direct child, a child-of-child, etc all the way up.
 *
 *   This is literally a copy of isaparentof except the args are switched.
 *   But a whole new file allows different help, etc
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
#include "pid_utils.h"

const volatile char *version = "0.1.0";
const volatile char *copyright = "isachildof - Copyright (c) 2017 Tim Savannah.";

/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: isachildof [child pid] [potential parent pid]\n", stderr);
    fputs("  Checks if 'child pid' is a child of any level for 'potential parent pid'\n", stderr);
}

/**
 * main - takes two arguements, child and check pid.
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

    if ( strncmp("--help", argv[1], 6) == 0 )
    {
        usage();
        return 0;
    }
    
    if ( strncmp("--version", argv[1], 9) == 0 )
    {
        fprintf(stderr, "\nisachildof version %s by Timothy Savannah\n\n", version);
        return 0;
    }

    ppid = strtoint(argv[2]);
    if ( ppid <= 0 )
    {
        fprintf(stderr, "Parent PID is not a valid integer: '%s'\n", argv[1]);
        return 1;
    }

    checkPid = strtoint(argv[1]);
    if ( ppid <= 0 )
    {
        fprintf(stderr, "Check PID is not a valid integer: '%s'\n", argv[2]);
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
        // Success, first-order child
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
