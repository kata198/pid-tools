/*
 * Copyright (c) 2017, 2018 Timothy Savannah All Rights Reserved
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
    fputs("Usage: waitpid [pid1] (Optional: [pid2] [pid...N])\n", stderr);
    fputs("  Waits for a given set of pids to finish.\n\nReturns 0 after pid terminates,\n  or 127 if provided pid does not exist.\n\nn", stderr);
}

#define USEC_IN_SECOND 1000000

#define POLL_TIME ( USEC_IN_SECOND / 100.0 )

#define ERR_NONE (0)
#define ERR_INVALID_PID_FORMAT (1)
#define ERR_NO_SUCH_PID (2)


#define MAX_PROC_PATH_SIZE (64)
/**
 * create_proc_path - Allocate a string of #MAX_PROC_PATH_SIZE and set contents to '/proc/'
 *          Used in construction of proc paths
 *
 *
 *      @return <char *> - An allocated string of size #MAX_PROC_PATH_SIZE with copied in '/proc/'
 */
static char *create_proc_path(void)
{
    static char baseStr[8] = { '/', 'p', 'r', 'o', 'c', '/' };

    char *ret;

    ret = malloc(MAX_PROC_PATH_SIZE);

    strncpy(ret, baseStr, 6);

    return ret;
}


/**
 * setup_proc_path - Convers a pid string to integer and appends to the path pointed-by #procPath
 *
 *
 *      @param procPath <char *> - Pointer to an allocated string which starts with '/proc/'
 *
 *      @param pidStr <const char *> - Pointer to a string of the pid
 *
 *      @param pidOut <int *> - Pointer to an integer which will be set with the
 *                      integer value of #pidStr.
 *
 *      @return <int> - If ERR_NONE (0) - Success
 *                      If ERR_INVALID_PID_FORMAT (1) - #pidStr is not a valid integer
 *                      IF ERR_NO_SUCH_PID (2) - Requested pid does not exist
 */
static unsigned int setup_proc_path(char *procPath, const char* pidStr, pid_t *pidOut)
{
    static pid_t pid;

    pid = *pidOut = strtoint(pidStr);
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

    static char **procPaths;
    static char *procPath;
    static int *inodeNums;
    static int curInode;
    static unsigned int tmp;
    static unsigned int i;
    static unsigned int numArgs;
    static pid_t curPid;

    static int keepGoing = 1;

    int ret = 0;

    if ( argc < 2 ) {
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

    numArgs = argc - 1;

    /* Gather all inodes and proc paths.
     *  We will check these in every loop iteration, and if 
     *   the inode is unavailable or has changed, the process has died / been replaced.
    */
    inodeNums = malloc(sizeof(int) * numArgs );
    procPaths = malloc(sizeof(char*) * numArgs );

    for(i=1; i <= numArgs; i++)
    {
        procPaths[i - 1] = procPath = create_proc_path();

        tmp = setup_proc_path(procPaths[i - 1], argv[i], &curPid);

        if ( unlikely( tmp != ERR_NONE ) )
        {
            switch(tmp)
            {
                case ERR_INVALID_PID_FORMAT:
                    fprintf(stderr, "Invalid pid: %s\n", argv[1]);
                    if ( ret < 1)
                        ret = 1;
                    /*goto __cleanup_exit__main;*/
                    break;
                case ERR_NO_SUCH_PID:
                    ret = 127;
                    /*goto __cleanup_exit__main;*/
                    break;
                default:
                    fprintf(stderr, "Unexpected return from setup_proc_path!\n");
                    if ( ret < 1)
                        ret = 1;
                    /*goto __cleanup_exit__main;*/
                    break;
            }

            /* Free and NULL this slot if we are in error. */
            free(procPaths[i - 1]);
            procPaths[i - 1] = NULL;
            inodeNums[i - 1] = -1;
        }
        else
        {
            inodeNums[i - 1] = get_inode_by_path(procPath);
        }

    }

    do {
        usleep( POLL_TIME );

        keepGoing = 0;

        for(i = 0; i < numArgs; i++)
        {
            /* Check each pid path for a matching inode */
            procPath = procPaths[i];
            if ( procPath == NULL )
                continue;

            curInode = get_inode_by_path(procPath);

            if ( curInode == inodeNums[i] ) {
                keepGoing = 1;
                break;
            }
            else
            {
                /* This process has quit and maybe a new process already has
                 *   the same pid. Don't bother checking it again.
                 */
                free(procPaths[i]);
                procPaths[i] = NULL;
            }

        }

    } while( keepGoing == 1 );


/*__cleanup_exit__main:*/

    free(inodeNums);
    for(i = 0; i < numArgs; i++)
    {
        if ( procPaths[i] != NULL )
            free(procPaths[i]);
    }
    free(procPaths);

    return ret;

}
