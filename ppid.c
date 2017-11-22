/*
 * Copyright (c) 2016, 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * ppid.c - Function to get the parent pid of a given pid. This
 *   is included statically by ppid.h unless SHARED_LIB is defined.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#include "pid_tools.h"

#include "ppid.h"


#define PROC_STAT_PPID_IDX 3

/*
 * getPpid - Gets the parent process ID of a provided pid.
 *
 * If no parent id is present, "1" (init) is returned. This includes
 * for pid 1 itself.
 *
 *  Returns "0" on error, and prints an error message
 *
 * pid - Search for parent of this pid.
 */
ALWAYS_INLINE_EXE_ONLY pid_t getPpid(pid_t pid)
{
    /* path - Array which will point to string: "/proc/$PID/stat" */

    static char path[64] = { '/', 'p', 'r', 'o', 'c', '/' };

    /* _buff - Short buffer. We only need to read the first couple fields, so 128 characters is plenty. */
    static char _buff[128];
    /* buff - Pointer to _buff which we modify address */
    char *buff;

    int fd;
    pid_t ret;

    sprintf(&path[6], "%u/stat", pid);

    fd = open(path, O_RDONLY);
    if ( fd <= 0 ) {
        return 0;
    }

    buff = _buff;

    buff[0] = '\0';

    if ( read(fd, buff, 127) <= 0 ) {
        /* Failed to read from "stat" */
        fprintf(stderr, "Error trying to read from '%s' [%d]: %s\n", buff, errno, strerror(errno));
    }

    close(fd);

    for(unsigned int numSpaces=0; numSpaces < PROC_STAT_PPID_IDX; buff = &buff[1] )
    {
        if ( *buff == ' ' ) {
            numSpaces++;
        }
    }

    {
      unsigned int nextIdx = 1;
      for( ; buff[nextIdx] != ' '; nextIdx++);

      buff[nextIdx] = '\0';
    }

    ret = atoi(buff);
    /* No parent means init is parent */
    if(ret == 0)
        ret = 1;
    return ret;

}

