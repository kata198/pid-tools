/*
 * pid_tools - Copyright (c) 2016, 2017 Timothy Savannah All Rights Reserved
 *   Licensed under terms of GNU General Purpose License (GPL) version 2
 */
#ifndef _PID_TOOLS_PPID_H
#define _PID_TOOLS_PPID_H

#include "pid_tools.h"

#include <sys/types.h>

/*
 *  Generally, from an executable, we directly just include the .c here (to allow best compiler optimization.
 *
 *  If you define SHARED_LIB, you'll need to link your .o with ppid.o
*/

INLINE_EXE_ONLY pid_t getPpid(pid_t pid);

#ifndef SHARED_LIB
#include "ppid.c"
#endif


#endif
