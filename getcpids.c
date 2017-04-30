/*
 * Copyright (c) 2016 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * getcpids.c - "main" for "getcpids" application -
 *  Gets and prints the child pids for a given pid, if any.
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
#include <dirent.h>
#include <ctype.h>

#include "pid_tools.h"
#include "pid_utils.h"

#include "ppid.h"

const volatile char *version = "0.1.0";
const volatile char *copyright = "getcpids - Copyright (c) 2016 Tim Savannah.";

static inline void usage()
{
    fputs("Usage: getcpids [pid]\n", stderr);
    fputs("  Prints the child process ids (pids) belonging to a given pid.\n", stderr);
}

/* 32 bytes on 64-bit system */
typedef struct {
    pid_t pid1;
    pid_t pid2;
    pid_t pid3;
    pid_t pid4;
    pid_t pid5;
    pid_t pid6;

    void *next;
}compound_pids;

/* CP_NEXT - Returns the "next" compound_pids, cast correctly */
#define CP_NEXT(cp) ((compound_pids *)cp->next)

/* cp_init - 
 *  Create a compound_pids object, zeroed-out.
 */
static inline compound_pids* cp_init(void)
{
    compound_pids *ret;

    ret = (compound_pids *) calloc(sizeof(compound_pids), 1);
    return ret;
}

/*
 * cp_extend - Extends a compound_pids struct by filling in "next".
 *
 *   Returns the extension.
 */
static compound_pids* cp_extend(compound_pids *toExtend)
{
    compound_pids *extension;

    extension = cp_init();
    toExtend->next = extension;

    return extension;
}

/*
 * cp_add - Add an element to the provided compound_pids struct, #cp, and if necessary
 *   extend it.
 *
 *   Returns the next compound_pids for adding (so "cp" if there is unused space still,
 *     otherwise the added extension is returned. )
 *
 *     offset - int pointer to current offset within compound_pids structure.
 *       This will be incremented as each item is added, and reset when extended.
 *       First add on an empty compound_pids structure should be '0'
 *
 *     pid - pid_t to add to cp or its extension (if extension is required)
 */
static inline compound_pids* cp_add(compound_pids *cp, unsigned int *offset, pid_t pid)
{
    if ( *offset == 6 )
    {
        cp = cp_extend(cp);

        *offset = 0;
    }

    pid_t *toSet = ((pid_t *)cp) + *(offset);

    *toSet = pid;
    *offset += 1;

    return cp;
}

/*
 * cp_to_list - Convert a compound_pids structure and all extensions
 *   into a null-terminated array.
 *
 *   Returns an allocated pointer to a pid_t array, with the final
 *    entry being "0" (not a valid pid).
 *
 *   cp - The head compound_pids structure
 *
 *   numEntries - The number of entries in cp and all extensions.
 *
 *   NOTE: since "numEntries" is required, null-terminating really
 *     isn't required. 
 *
 *     This makes it simpler if one-day I decide to turn this stuff
 *     into a shared-object. "numEntries" could then be calculated by
 *     a yet-existing "cp_len" function.
 *
 *     I prefer to leave it null-terminated, so that it may have use
 *     in external functions which rely on such and don't take a 
 *     length param, rather than relying on the user realloc'ing in this
 *     case.
 */
static pid_t* cp_to_list(compound_pids *cp, unsigned int numEntries)
{
    pid_t *ret, *curPid, *retPtr;
    compound_pids *cur;
    unsigned int i;

    ret = malloc(sizeof(pid_t) * (numEntries + 1));
    retPtr = ret;
    
    cur = cp;

    do {
        curPid = &cur->pid1;
        for(i=0, curPid = &cur->pid1; i < 6; i++, curPid += 1, retPtr += 1)
        {
            *retPtr = *(curPid);
            if ( *retPtr == 0 )
                goto after_loop;
        }

        cur = CP_NEXT(cur);
    }while( !!(cur) );
    
after_loop:

    return ret;
}


/**
 * main - Takes one argument, the pid. Will scan
 *    all accessable pids on the system, and print 
 *    a space-separated list of child pids (of next level)
 *
 *    TODO: Check for --help
 */
int main(int argc, char* argv[])
{

    compound_pids *cpList, *curCp;

    unsigned int cpOffset;
    DIR *procDir;
    struct dirent *dirInfo;
    pid_t providedPid;
    pid_t nextPid;
    char* nextPidStr;
    pid_t ppid;

    pid_t *printList;
    unsigned int numItems;


    if ( argc != 2 ) {
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }


    providedPid = strtoint(argv[1]);
    if ( providedPid <= 0 )
    {
        fprintf(stderr, "Invalid pid: %s\n", argv[1]);
        return 1;
    }

    cpList = cp_init();

    cpOffset = 0;
    numItems = 0;
    curCp = cpList;

    procDir = opendir("/proc");
    while( (dirInfo = readdir(procDir)) )
    {
            nextPidStr = dirInfo->d_name;
            if(!isdigit(nextPidStr[0]))
                    continue;

            nextPid = atoi(nextPidStr);
            ppid = getPpid(nextPid);
            if(ppid == providedPid)
            {
                curCp = cp_add(curCp, &cpOffset, nextPid);
                numItems++;
            }
    }
    /* No children. */
    if ( cpList->pid1 == 0 )
        return 0;


    printList = cp_to_list(cpList, numItems);

    pid_t *curPtr;

    for(curPtr = printList ; ; )
    {
        printf("%d", *curPtr);
        curPtr += 1;
        if ( *curPtr == 0 )
            break;
        putchar(' ');
    }
    putchar('\n');

    return 0;


}
