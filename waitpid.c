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
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "pid_tools.h"

#include "pid_utils.h"
#include "pid_inode_utils.h"
 
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

#define ERR_NONE (0)
#define ERR_INVALID_PID_FORMAT (1)
#define ERR_NO_SUCH_PID (2)

static unsigned int setup_proc_path(char *procPath, const char* pidStr)
{
    static pid_t pid;

    pid = strtoint(pidStr);
    if ( pid <= 0 )
    {
        return ERR_INVALID_PID_FORMAT;
    }


    sprintf(&procPath[6], "%d", pid);
    if ( access( procPath, F_OK ) != 0 )
    {
        /* Pid does not exist... */
        return ERR_NO_SUCH_PID;
    }

    return ERR_NONE;
}

/**
 * main - takes one argument, the pid to wait on
 *
 */
int main(int argc, char* argv[])
{

    static char procPath[64] = { '/', 'p', 'r', 'o', 'c', '/' };
    static int inodeNum;
    static int curInode;
    static unsigned int tmp;

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

    tmp = setup_proc_path(procPath, argv[1]);

    if ( unlikely( tmp != ERR_NONE ) )
    {
        switch(tmp)
        {
            case ERR_NONE:
                break;
            case ERR_INVALID_PID_FORMAT:
                fprintf(stderr, "Invalid pid: %s\n", argv[1]);
                return 1;
                break;
            case ERR_NO_SUCH_PID:
                return 127;
                break;
            default:
                fprintf(stderr, "Unexpected return from setup_proc_path!\n");
                return 1;
                break;
        }
    }

    if ( access( procPath, F_OK ) != 0 )
    {
        /* Pid does not exist... */
        return 127;
    }

    inodeNum = curInode = get_inode_by_path(procPath);
    if ( inodeNum == -1 )
    {
        /* Cannot stat. */
        return 127;
    }
    
    
    do {
        usleep ( POLL_TIME );
        curInode = get_inode_by_path(procPath);
    }while( access( procPath, F_OK) == 0 && inodeNum == curInode);


    return 0;

}
