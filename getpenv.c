/*
 * Copyright (c) 2016, 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * getpenv.c - "main" for "getpenv" application -
 *  Gets and prints an env variable for a given pid
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

const volatile char *copyright = "getpenv - Copyright (c) 2016, 2017 Tim Savannah.";

/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: getpenv [pid] [env var name]\n", stderr);
    fputs("  Prints the value of an env var as set for given pid\n\n", stderr);
    fputs("Return code is 254 if no such name in the environ of given process\n Otherwise is non-zero indicating error (in case of error).\n\n", stderr);
    fputs("Example: getpenv 12345 PATH\n\n", stderr);
}

#define NOT_FOUND ( (char*) 1)

/*
static int doesPidExist(pid_t pid)
{
    static char tmp[32] = { '/', 'p', 'r', 'o', 'c', '/', 0 };
    static struct stat statbuf;

    sprintf( &tmp[6], "%d", pid);

    if ( stat(tmp, &statbuf) != 0 )
        return 0;

    return 1;
}
*/

static int isMatch(const char *possibleMatch, const char *checkAgainst)
{
    size_t i;

    for( i=0; checkAgainst[i] != '\0'; i++)
    {
        if ( possibleMatch[i] == '\0' )
        {
            /* If other string ended before we matched */
            return 0;
        } else if ( possibleMatch[i] != checkAgainst[i] )
        {
            /* If current index does not match */
            return 0;
        }
    }
    
    /* We matched it all */
    return 1;
}


#define READ_DATA_INCR_BUFSIZ 65535

static size_t readData(char **buf, FILE *envFile)
{
    size_t curSize = READ_DATA_INCR_BUFSIZ;
    size_t bytesRead;
    size_t totalBytesRead = 0;

    char *cur;

    *buf = malloc(curSize + 1);

    cur = *buf;

_read_again:

    bytesRead = fread(cur, 1, READ_DATA_INCR_BUFSIZ, envFile);

    totalBytesRead += bytesRead;

    if ( bytesRead == READ_DATA_INCR_BUFSIZ || !feof(envFile) )
    {
        curSize += READ_DATA_INCR_BUFSIZ;

        *buf = realloc(*buf, curSize);

        cur = &buf[0][totalBytesRead];
        goto _read_again;
    }

    return totalBytesRead;
}


static char* getEnvValueForPid(pid_t pid, const char* envName)
{
    static char path[64] = { '/', 'p', 'r', 'o', 'c', '/', 0 };
    
    char *buf, *cur, *val, *ret;
    FILE *envFile;
    size_t idx, maxIdx, thisLen, envNameLen;

    /* "Not Found" marker */
    ret = NOT_FOUND;

    sprintf( &path[6], "%d/environ", pid );

    envFile = fopen(path, "r");
    if ( envFile == NULL )
    {
//        errno = ESRCH;
        return NULL;
    }

    maxIdx = readData(&buf, envFile);

    fclose(envFile);

    envNameLen = strlen(envName);
    cur = buf;
    idx = 0;


    while( idx < maxIdx )
    {
        thisLen = strlen(cur);

        if( !isMatch(cur, envName) || cur[envNameLen] != '=' )
        {
            cur = &cur[thisLen + 1];
            idx += thisLen + 1;
            continue;
        }
        val = cur + envNameLen + 1;

        ret = malloc(thisLen - envNameLen);
        strcpy(ret, val);

        break;
    }

    free(buf);

    return ret;
}

/**
 * main - takes two arguments, the pid and env var name
 *
 */
int main(int argc, char* argv[])
{

    pid_t pid;

    unsigned int i;
    size_t envNameLen;
    char *envName, *envVal;

    int ret;

    ret = 0;

    for( i=1; i < argc; i++)
    {
        if ( strncmp("--help", argv[i], 6) == 0 )
        {
            usage();
            return 0;
        }
        
        if ( strncmp("--version", argv[i], 9) == 0 )
        {
            fprintf(stderr, "\ngetpenv version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
            return 0;
        }
    }


    if ( argc != 3 ) {
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }

    pid = strtoint(argv[1]);
    if ( pid <= 0 )
    {
        fprintf(stderr, "Invalid pid: %s\n", argv[1]);
        return 1;
    }
/*
    if ( !doesPidExist(pid) )
    {
        fprintf(stderr, "Cannot access pid: %d (Not running?)\n", pid);
        return 1;
    }
*/

    /* Copy into envName argv[2] */
    envNameLen = strlen(argv[2]);

    envName = malloc( envNameLen + 1 );
    strcpy(envName, argv[2]);

    envVal = getEnvValueForPid(pid, envName);

    if( unlikely(envVal == NOT_FOUND ) )
    {
        ret = 254;
        return 254;
    }

    else if ( unlikely(envVal == NULL) )
    {
        ret = errno;

        fprintf(stderr, "Error reading env var '%s' from pid=%d. Error %d: %s\n", envName, pid, errno, strerror(errno));
        
    }
    else
    {
        puts(envVal);
        
        free(envVal);
    }


    free(envName);

    return ret;

}
