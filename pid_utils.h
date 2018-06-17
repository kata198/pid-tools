/*
 * Copyright (c) 2017 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * pid_utils.h - some general-purpose static utility functions
 *         shared by several executables
 *
 *         These are contained in this header versus a .c file to allow
 *         optimizations which wouldn't otherwise get applied if not single unit 
 *         (e.x. inlining).
 *
 *   This is literally a copy of isaparentof except the args are switched.
 *   But a whole new file allows different help, etc
 */

#ifndef _PPID_UTILS_H
#define _PPID_UTILS_H

#include "pid_tools.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>

static pid_t strtoint(const char *str)
{
    pid_t ret;
    char *endptr = { 0 };

    errno = 0;

    if ( likely( *str ) )
    {
        ret = strtol(str, &endptr, 10);
    }
    else
    {
        errno = 1; /* Simulate error */
        return 0;
    }

    if ( unlikely( errno != 0 || ( !ret && endptr == str) || ( *endptr != '\0' ) ) )
    {
        errno = 1;
        return 0;
    }

    return ret;
}

#endif
