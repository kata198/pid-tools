/*
 * Copyright (c) 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * getpcmd.c - "main" for "getpcmd" application -
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
#include "pid_utils.h"

const volatile char *version = "0.1.0";
const volatile char *copyright = "getpcmd - Copyright (c) 2016 Tim Savannah.";

/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: getpcmd [pid]\n", stderr);
    fputs("  Prints the commandline string of a given pid\n", stderr);
}


static char *read_commandline(pid_t pid)
{
    char *path;
    int fd;
    char *ret = NULL;
    ssize_t size, bytesRead;

    long int curBuffSize = 4096 << 1;

    path = malloc(128);

    sprintf(path, "/proc/%d/cmdline", pid);

    errno = 0;

    fd = open(path, O_RDONLY);
    free(path);

    if( unlikely(fd == -1 || errno) )
    {
        goto cleanup_err;
    }

    /* fseek doesn't seem to work on proc to get file size.. */


    ret = malloc(curBuffSize + 1); /* This is overestimate, because of nulls at the end of each arg. */

    size = 0;

    char *ptr;

    long nextSize = curBuffSize;

    bytesRead = 0;
    *ret = 0;

    /* Start with 8192 max size, and keep doubling while we consume the commandline string. */
    do 
    {
        ptr = ret + size;
        bytesRead = read(fd, ptr, nextSize);
        if ( unlikely ( !bytesRead ) )
        {
            close(fd);
            goto cleanup_err;
        }
        else if ( bytesRead < nextSize )
        {
            size += bytesRead;
            ret[size + 1] = '\0';
            break;
        }
        else
        {
            size += bytesRead;
            curBuffSize = curBuffSize << 1;

            ptr = malloc(curBuffSize + 1);
            memcpy(ptr, ret, size);
            free(ret);

            ret = ptr;
            ptr = ret + size;

            nextSize = curBuffSize - size;
        }
    }while(1);
            
    close(fd);


	/* Replace all null (arg separater) with space */
    register char *curPtr;

    curPtr = ret;

	while ( ( curPtr - ret ) < size )
    {
        if ( unlikely( *curPtr == '\0' ) )
		{
			*curPtr = ' ';
		}
		curPtr++;
	}


    return ret;

cleanup_err:

    fprintf(stderr, "Error, pid %d does not exist or is not accessable.\n", pid);
    if ( ret != NULL )
    {
        free(ret);
    }
    return NULL;


}

/**
* main - takes one argument, the search pid.
 *
 */
int main(int argc, char* argv[])
{

    pid_t pid;

    char *cmdlineStr;


    if ( unlikely (argc != 2 ) )
    { 
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }


    pid = strtoint(argv[1]);
    if ( unlikely(pid <= 0 ) )
    {
        fprintf(stderr, "Invalid pid: %s\n", argv[1]);
        return 1;
    }

    cmdlineStr = read_commandline(pid);
    if ( unlikely(cmdlineStr == NULL ) )
    {
        return 1;
    }


    printf("%s\n", cmdlineStr);

    return 0;

}
