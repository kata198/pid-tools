/*
 * Copyright (c) 2016, 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * getppid.c - "main" for "getppid" application -
 *  Gets and prints the parent pid of a given pid
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

const volatile char *copyright = "getppid - Copyright (c) 2016, 2017 Tim Savannah.";

/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: getppid [pid]\n", stderr);
    fputs("  Prints the parent process id (PPID) for a given pid.\n", stderr);
}

/**
 * main - takes one argument, the search pid.
 *
 * Prints the child's parent pid to stdout. If child has no parent,
 *  1 is printed (init). This includes when "1" is queried, i.e. "ppid(1) = 1"
 *
 *  TODO: Check for --help
 */
int main(int argc, char* argv[])
{

    pid_t pid, ppid;


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
        fprintf(stderr, "\ngetppid version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
        return 0;
    }

    pid = strtoint(argv[1]);
    if ( pid <= 0 )
    {
        fprintf(stderr, "Invalid pid: %s\n", argv[1]);
        return 1;
    }

    ppid = getPpid(pid);

    printf("%u\n", ppid);

    return 0;

}
