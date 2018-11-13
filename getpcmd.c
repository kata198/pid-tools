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
    fputs("Usage: getpcmd (Options) [pid] (Optional: [pid2] [pid3])\n", stderr);
    fputs("  Prints the commandline string of given pids\n", stderr);
    fputs("\n  Options:\n\n     --quote              Quote the command arguments in output\n\n", stderr);
}

/**
 * do_print_commandline - Print the value of the proc cmdline contents, optionally quoting each argument.
 */
static void do_print_commandline(char *ptr, ssize_t size, int quoteArgs);

/**
 *   read_and_print_proc_cmdline - Read the "cmdline" property of a given pid, 
 *                        and either print it or an error message.
 *
 *       Returns 1 on success, 0 on error
*/
static int read_and_print_proc_cmdline(pid_t pid, int quoteArgs)
{
    static char path[64] = { '/', 'p', 'r', 'o', 'c', '/' };
    int fd;
    char *ret = NULL;
    ssize_t size, bytesRead;

    long int curBuffSize = 4096 << 1;

    sprintf(&path[6], "%d/cmdline", pid);

    errno = 0;

    fd = open(path, O_RDONLY);

    if( unlikely(fd == -1 || errno) )
    {
        goto cleanup_err_exit;
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

    free(ret);


    return 1;

/* cleanup_err - Free "ret", print error message and exit=0 */
cleanup_err:

    free(ret);

/* cleanup_err_exit - Just print error message and exit=0 (go directly here if ret not allocated) */
cleanup_err_exit:

    fprintf(stderr, "Error, pid %d does not exist or is not accessable.\n", pid);
    return 0;


}


/**
 * do_print_commandline - Print the value of the proc cmdline contents, optionally quoting each argument.
 */
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
    pid_t *pids;
    unsigned int numPids = 0;
    int quoteArgs = 0;
    int i;
    int ret = 0;


    /* PARSE ARGS */
    if ( unlikely (argc < 2 ) )
    {
_invalid_arg_exit:
        fprintf(stderr, "Too few arguments. Run `%s --help' to see usage.\n\n", argv[0]);
        usage();
        return 1;
    }

    pids = alloca(sizeof(pid_t) * (argc-1));

    for(i=1; i < argc; i++)
    {
        char *arg = argv[i];

        if ( unlikely(strncmp("-h", arg, 2) == 0 || strncmp("--help", arg, 6) == 0) )
        {
            usage();
            return 0;
        }
        if ( unlikely(strncmp("--version", arg, 9) == 0) )
        {
            fprintf(stderr, "getpcmd version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
            return 0;
        }


        if ( unlikely(strncmp("--quote", arg, 7) == 0) )
        {
            if ( unlikely(argc == 2) )
            {
                goto _invalid_arg_exit;
            }

            quoteArgs = 1;
            continue;
        }


        /* Convert and validate provided "pid" argument */
        pid = strtoint(arg);
        if ( unlikely(pid <= 0 ) )
        {
            fprintf(stderr, "Provided PID is not a valid integer: '%s'\n", arg);
            return 1;
        }
        pids[numPids++] = pid;
    }

    if( unlikely(numPids <= 0) )
    {
        fprintf(stderr, "Missing pid argument. See `%s --help' for usage.\n", argv[0]);
        return 1;
    }

    for(i=0; i < numPids; i++)
    {
        /* Read and print the contents of the proc cmdline for this pid,
         *   and exit with error(1) or success (0)
         */
        if ( unlikely( !read_and_print_proc_cmdline(pids[i], quoteArgs) ) )
        {
            ret = 1;
        }
    }

    return ret;

}
