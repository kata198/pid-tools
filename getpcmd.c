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

#include "pid_tools.h"

#include "ppid.h"
#include "pid_utils.h"

const volatile char *copyright = "getpcmd - Copyright (c) 2017 Tim Savannah.";

/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: getpcmd (Options) [pid]\n", stderr);
    fputs("  Prints the commandline string of a given pid\n", stderr);
    fputs("\n  Options:\n\n     --quote              Quote the command arguments in output\n\n", stderr);
}

static void do_print_commandline(char *ptr, ssize_t size, int quoteArgs);

static int read_commandline(pid_t pid, int quoteArgs)
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

    /* Empty commandline str? */
    if ( size == 0 )
    {
        puts("\n");
    }
    else
    {
        do_print_commandline(ret, size, quoteArgs);
    }

    return 1;

cleanup_err:

    fprintf(stderr, "Error, pid %d does not exist or is not accessable.\n", pid);
    if ( ret != NULL )
    {
        free(ret);
    }
    return 0;


}


static void do_print_commandline(char *ptr, ssize_t size, int quoteArgs)
{
    register char *curPtr;

    curPtr = ptr;
    if ( quoteArgs )
    {
        do
        {
            putchar('"');
            while( *curPtr != '\0' )
            {
                if ( *curPtr == '"' )
                    putchar('\\');
                else if ( *curPtr == '\\' )
                    putchar('\\');
                putchar(*curPtr);
                curPtr ++;
            }
            putchar('"');
            curPtr ++;

            if ( curPtr - ptr >= size )
                break;
            putchar(' ');

        }  while ( 1 );
    }
    else
    {
        do
        {
            printf("%s", curPtr);
            do
            {
                curPtr ++;
            } while( *curPtr != '\0' );

            curPtr ++;

            if ( curPtr - ptr >= size )
                break;
            putchar(' ');

        }  while ( 1 );
    }


    putchar('\n');
}


/**
* main - takes one argument, the search pid.
 *
 */
int main(int argc, char* argv[])
{

    pid_t pid;
    int quoteArgs = 0;
    char *pidStrPtr;


    if ( unlikely (argc != 2 && argc != 3 ) )
    {
_invalid_arg_exit:
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }

    if ( unlikely(strncmp("-h", argv[1], 2) == 0 || strncmp("--help", argv[1], 6) == 0) )
    {
        usage();
        return 0;
    }
    if ( unlikely(strncmp("--version", argv[1], 9) == 0) )
    {
        fprintf(stderr, "getpcmd version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
        return 0;
    }


    if ( unlikely(strncmp("--quote", argv[1], 7) == 0) )
    {
        if ( unlikely(argc == 2) )
        {
            goto _invalid_arg_exit;
        }

        quoteArgs = 1;
        pidStrPtr = argv[2];
    }
    else
    {
        if( unlikely(argc > 2) )
        {
            fputs("Too many arguments or unknown option.\n\n", stderr);
            usage();
            return 1;
        }
        quoteArgs = 0;
        pidStrPtr = argv[1];
    }


    pid = strtoint(pidStrPtr);
    if ( unlikely(pid <= 0 ) )
    {
        fprintf(stderr, "Provided PID is not a valid integer: '%s'\n", pidStrPtr);
        return 1;
    }

    if ( unlikely( !read_commandline(pid, quoteArgs) ) )
    {
        return 1;
    }


    return 0;

}
