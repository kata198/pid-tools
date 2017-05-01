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

#include "pid_tools.h"

#include "ppid.h"
#include "pid_utils.h"

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

    if ( strncmp("--help", argv[1], 6) == 0 )
    {
        usage();
        return 0;
    }
    
    if ( strncmp("--version", argv[1], 9) == 0 )
    {
        fprintf(stderr, "\nisaparentof version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
        return 0;
    }

    ppid = strtoint(argv[1]);
    if ( ppid <= 0 )
    {
        fprintf(stderr, "Parent PID is not a valid integer: '%s'\n", argv[1]);
        return 1;
    }

    checkPid = strtoint(argv[2]);
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
