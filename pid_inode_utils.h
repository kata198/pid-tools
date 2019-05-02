/*
 * Copyright (c) 2018 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * pid_inode_utils.h - some static utility functions shared by several executables
 *            related to checking inodes.
 *
 *         These are contained in this header versus a .c file to allow
 *         optimizations which wouldn't otherwise get applied if not single unit 
 *         (e.x. inlining).
 *
 */

#ifndef _PID_INODE_UTILS_H
#define _PID_INODE_UTILS_H

#include "pid_tools.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/** Potential error returns: */

/* PIU_ERROR_FSTAT - The fstat call failed */
#define PIU_ERROR_FSTAT (-1)
/* PIU_ERROR_OPEN - The open call failed */
#define PIU_ERROR_OPEN  (-2)



/* get_inode_by_filedes - Returns the inode associated with a file descriptor
 *
 *   @param filedes <int> - an open file descriptor (returned by open[2])
 *
 *     @return <int> - If > 0 - The inode represented by the file descriptor
 *                     If -1  - A failure was returned by the `fstat' call.
 *                                errno may contain the reason if libc set it
 *
 *     NOTE: Due to moves or overwrites, the object associated with the "path"
 *             used to open this file may no longer match the object #filedes references.
 *
 *           So while this function is more efficent because it saves an open and close,
 *             if the object you are tracking could have moved or been replaced,
 *             and this is a problem, see #get_inode_by_path
 */
MAYBE_UNUSED static int get_inode_by_filedes(int fileDes)
{
    static struct stat statBuf;
    int inode;
    int oldErrno;

    if ( unlikely( fstat(fileDes, &statBuf) < 0 ) )
    {
        oldErrno = errno; /* close might erase errno */
        close(fileDes);
        errno = oldErrno;
        return PIU_ERROR_FSTAT;
    }

    inode = statBuf.st_ino;

    return inode;
}

/* get_inode_by_path - Returns the inode associated with a filesystem object
 *                        at the given path.
 *
 *   @param filePath <const char*> - a NULL-terminated path to the
 *                            filesystem object of interest.
 *
 *     @return <int> - If > 0 - The inode represented by the file descriptor
 *                     If -1  - A failure was returned by the `fstat' call.
 *                                errno may contain the reason if libc set it
 *                     If -2  - A failure was returned by the `open' call.
 *                                errno may contain the reason if libc set it
 *
 *
 *         NOTE: This function is slightly more expensive than #get_inode_by_filedes ,
 *            but it works when a replaced object is to be treated the same as
 *            an empty spot.
 */
MAYBE_UNUSED static int get_inode_by_path(const char* filePath)
{
    static int fileDes = -1;
    static struct stat statBuf;
    int oldErrno;
    int inode;

    fileDes = open(filePath, O_RDONLY);
    if ( unlikely( fileDes < 0 ) )
        return PIU_ERROR_OPEN;

    if ( unlikely( fstat(fileDes, &statBuf) < 0 ) )
    {
        oldErrno = errno; /* close might erase errno */
        close(fileDes);
        errno = oldErrno;
        return PIU_ERROR_FSTAT;
    }

    inode = statBuf.st_ino;
    close(fileDes);

    return inode;
}

#endif
