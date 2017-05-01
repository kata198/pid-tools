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


/*
 * getPpid - Gets the parent process ID of a provided pid.
 *
 * If no parent id is present, "1" (init) is returned. This includes
 * for pid 1 itself.
 *
 * pid - Search for parent of this pid.
 */
ALWAYS_INLINE_EXE_ONLY pid_t getPpid(pid_t pid)
{
    char _buff[128] = { '/', 'p', 'r', 'o', 'c', '/' };
    char *buff = _buff;
    int fd;
    pid_t ret;

    sprintf(&buff[6], "%u/stat", pid);

    fd = open(buff, O_RDONLY);
    if ( fd <= 0 ) {
        return 0;
    }

    if ( read(fd, buff, 128) <= 0 ) {
        fprintf(stderr, "Error trying to read from '%s' [%d]: %s\n", buff, errno, strerror(errno));
    }
    close(fd);    
    for(unsigned int numSpaces=0; numSpaces < 3; buff = &buff[1] )
    {
        if ( *buff == ' ' ) {
            numSpaces++;
        }
    }

    unsigned int nextIdx = 1;
    for( ; buff[nextIdx] != ' '; nextIdx++);

    buff[nextIdx] = '\0';

    ret = atoi(buff);
    /* No parent means init is parent */
    if(ret == 0)
        ret = 1;
    return ret;

}

