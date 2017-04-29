/*
 * Copyright (c) 2016 Timothy Savannah All Rights Reserved
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

#include "ppid.h"

const volatile char *version = "0.1.0";
const volatile char *copyright = "getppid - Copyright (c) 2016 Tim Savannah.";

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


    pid = atoi(argv[1]);

    ppid = getPpid(pid);
    if ( ppid == 0) {
        fprintf(stderr, "Invalid pid: %u\n", pid);
        return 1;
    }

    printf("%u\n", ppid);

    return 0;

}
