/*
 * Copyright (c) 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * isachildof.c - Checks if a given process is in any way connected as a child.
 *   This could be a direct child, a child-of-child, etc all the way up.
 *
 *   This is literally a copy of isaparentof except the args are switched.
 *   But a whole new file allows different help, etc
 */

#ifndef _PPID_UTILS_H
#define _PPID_UTILS_H

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

static pid_t strtoint(char *str)
{
    pid_t ret;
    char *endptr = { 0 };

    errno = 0;

    if ( *str )
    {
        ret = strtol(str, &endptr, 10);
    }
    else
    {
        errno = 1; /* Simulate error */
        return 0;
    }

    if ( errno != 0 || ( !ret && endptr == str) || ( *endptr != '\0' ) )
    {
        return 0;
    }

    return ret;
}

#endif
