/*
 * Copyright (c) 2016, 2017, 2018 Timothy Savannah All Rights Reserved
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

const volatile char *copyright = "getcpids - Copyright (c) 2016, 2017, 2018 Tim Savannah.";

static inline void usage()
{
    fputs("Usage: getcpids [pid] (Optional: [pid2] [pid..N])\n", stderr);
    fputs("  Prints the child process ids (pids) belonging to a given pid or pids.\n", stderr);
}


#define NUM_PIDS_IN_COMPOUND_PIDS 6

/* 32 bytes on 64-bit system (8-byte void pointer).
*/
typedef struct {
    pid_t pids[NUM_PIDS_IN_COMPOUND_PIDS] ALIGN_4;
    /*pid_t pid2;
    pid_t pid3;
    pid_t pid4;
    pid_t pid5;
    pid_t pid6;
*/
    void *next;
}compound_pids ALIGN_32;

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

/* cp_destroy -
 *   Unallocate a compound_pids object and all associated objects linked via "next"
 */
static void cp_destroy(compound_pids *cp)
{
    /* Recurse all the way to the last, and start freeing in reverse */
    if ( cp->next != NULL )
    {
        cp_destroy( (compound_pids*) cp->next );
    }

    free(cp);
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
    if ( *offset == NUM_PIDS_IN_COMPOUND_PIDS )
    {
        /* If we have exceeded the number of pids in this struct,
         *   we have to extend.
         *
         *  To extend, we allocate another compound_pids struct and point the current "next"
         *    pointer to this new structure.
         */
        cp = cp_extend(cp);

        *offset = 0;
    }

    /* Calculate the offset to the pid# we are trying to set */
    cp->pids[*offset] = pid;

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
    pid_t *ret, *retPtr;
    compound_pids *cur;
    unsigned int i;

    /* ret will normally get null terminated by assigning to a 0 cur->pids[i]
     *   In the case that all cur->pids have assigned pids but there is no next,
     *     we must explicitly zero the end
     */
    ret = malloc(sizeof(pid_t) * (numEntries + 1));
    retPtr = ret;
    
    cur = cp;

    do {
        for(i=0; i < NUM_PIDS_IN_COMPOUND_PIDS; i++, retPtr += 1)
        {
            *retPtr = cur->pids[i];
            if ( *retPtr == 0 )
                goto after_loop;
        }

        cur = CP_NEXT(cur);
    }while( !!(cur) );

    /* Handle the case where all pids were assigned in the last compound_pids struct,
     *   so we didn't assign the last *retPtr to 0
     */
    *retPtr = 0;

after_loop:

    return ret;
}


/**
 * main - Takes one argument, the pid. Will scan
 *    all accessable pids on the system, and print 
 *    a space-separated list of child pids (of next level)
 *
 */
int main(int argc, char* argv[])
{

    compound_pids *cpList, *curCp;

    unsigned int cpOffset;
    DIR *procDir;
    struct dirent *dirInfo;
    pid_t *providedPids = NULL;
    pid_t providedPid;
    pid_t nextPid;
    char* nextPidStr;
    pid_t ppid;

    pid_t *printList;
    unsigned int numItems;
    unsigned int i;
    unsigned int numArgs;

    int returnCode = 0;

    if ( argc < 2 ) {
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }


    numArgs = argc - 1;

    cpList = NULL;
    providedPids = malloc( sizeof(pid_t) * numArgs );

    for( i=1; i <= numArgs; i++ )
    {

        providedPid = strtoint(argv[i]);
        if ( providedPid <= 0 )
        {
            /* If we failed to convert, maybe it is an option */
            if ( strncmp("--help", argv[i], 6) == 0 )
            {
                usage();
                returnCode = 0;
                goto __cleanup_and_exit;
            }
            
            if ( strncmp("--version", argv[i], 9) == 0 )
            {
                fprintf(stderr, "\ngetcpids version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
                returnCode = 0;
                goto __cleanup_and_exit;
            }

            /* Nope -- fail out. */
            fprintf(stderr, "Invalid pid: %s\n", argv[i]);
            returnCode = 1;
            goto __cleanup_and_exit;
        }

        providedPids[i - 1] = providedPid;
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
            for( i=0; i < numArgs; i++ )
            {
                providedPid = providedPids[i];
                if(ppid == providedPid)
                {
                    curCp = cp_add(curCp, &cpOffset, nextPid);
                    numItems++;
                    break;
                }
            }
    }
    closedir(procDir);

    /* No children. */
    if ( cpList->pids[0] == 0 )
        goto __cleanup_and_exit;


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
    free(printList);

__cleanup_and_exit:

    if ( cpList != NULL )
        cp_destroy(cpList);

    if ( providedPids != NULL )
        free(providedPids);

    return returnCode;
}
