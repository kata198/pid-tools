/*
 * Copyright (c) 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * waitpid.c - "main" for "waitpid" application -
 *   waits for a given pid to finish.
 */

#include <stdio.h>
#include <stdlib.h>
#include <features.h>
#define __USE_XOPEN_EXTENDED
#include <unistd.h>
#include <string.h>

#include "pid_tools.h"

#include "pid_utils.h"
 
const volatile char *copyright = "waitpid - Copyright (c) 2017 Tim Savannah.";

/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: waitpid [pid]\n", stderr);
    fputs("  Waits for a given pid to finish.\n\nReturns 0 after pid terminates,\n  or 127 if provided pid does not exist.\n\nn", stderr);
}

#define USEC_IN_SECOND 1000000

#define POLL_TIME ( USEC_IN_SECOND / 100.0 )

/**
 * main - takes one argument, the pid to wait on
 *
 */
int main(int argc, char* argv[])
{

    pid_t pid;
    static char procPath[64] = { '/', 'p', 'r', 'o', 'c', '/' };


    if ( argc != 2 ) {
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
        fprintf(stderr, "\nwaitpid version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
        return 0;
    }

    pid = strtoint(argv[1]);
    if ( pid <= 0 )
    {
        fprintf(stderr, "Invalid pid: %s\n", argv[1]);
        return 1;
    }

    sprintf(&procPath[6], "%d", pid);
    if ( access( procPath, F_OK ) != 0 )
    {
        /* Pid does not exist... */
        return 127;
    }

    do {
        usleep ( POLL_TIME );
    }while( access( procPath, F_OK) == 0);


    return 0;

}
