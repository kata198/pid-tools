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

#include "simple_int_map.h"

#include "ppid.h"

const volatile char *copyright = "getcpids - Copyright (c) 2016, 2017, 2018 Tim Savannah.";

static inline void usage()
{
    fputs("Usage: getcpids (Options) [pid] (Optional: [pid2] [pid..N])\n", stderr);
    fputs("  Prints the child process ids (pids) belonging to a given pid or pids.\n\n", stderr);
    fputs("    Options:\n\t\t-r\t\tRecursive mode. Gets child pids, and their children, and so on.\n\n", stderr);
}


static int cmp_pids(const void *p1, const void *p2)
{
    pid_t val1, val2;

    val1 = *((pid_t *)p1);
    val2 = *((pid_t *)p2);

    return val1 - val2;
}

/* TODO: Investigate if we strip out from allPidsList when we add if we can speedup getcpids 1 -r more */
static void get_cpids_recursive(SimpleIntMap *matchedPidsMap, pid_t providedPid, pid_t *allPids, size_t allPidsLen)
{
    unsigned int i;

    static pid_t nextPid;
    static pid_t ppid;

    for( i=0; i < allPidsLen; i++ )
    {
        nextPid = allPids[i];
        if ( simple_int_map_contains( matchedPidsMap, nextPid ) )
            continue; /* Already added and recursed */

        ppid = getPpid(nextPid);
        if(ppid == providedPid)
        {
            /* Already known to not be present because of above check */
            simple_int_map_add( matchedPidsMap, nextPid );
            get_cpids_recursive( matchedPidsMap, nextPid, allPids, allPidsLen );
            /*
            if ( simple_int_map_add( matchedPidsMap, nextPid ) )
            {*/
                /* 
                   We successfully added this entry (wasn't already in list)
                    so recurse to add all children of matched pid 
                */
                /*
                get_cpids_recursive( matchedPidsMap, nextPid, allPids, allPidsLen );
            }
                */
        }
    }
}

/**
 * main - Takes one argument, the pid. Will scan
 *    all accessable pids on the system, and print 
 *    a space-separated list of child pids (of next level)
 *
 */
int main(int argc, char* argv[])
{
    SimpleIntMap *matchedPidsMap = NULL;

    SimpleIntMap *allPidsMap = NULL;
    pid_t *allPids = NULL;
    size_t allPidsLen = 0;

    DIR *procDir;
    struct dirent *dirInfo;
    pid_t *providedPids = NULL;
    pid_t providedPid;
    pid_t nextPid;
    char* nextPidStr;
    pid_t ppid;

    pid_t *printList;
    size_t numItems;
    unsigned int i, j;
    unsigned int numArgs; /* Total number of arguments */
    unsigned int numPidArgs; /* Total number of arguments that were pids */

    char isRecursiveMode = 0; /* Set to 1 in recursive mode */

    int returnCode = 0;


    if ( argc < 2 ) {
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }


    numArgs = argc - 1;
    numPidArgs = 0;

    providedPids = malloc( sizeof(pid_t) * numArgs );

    for( i=1; i <= numArgs; i++ )
    {

        providedPid = strtoint(argv[i]);
        if ( providedPid <= 0 )
        {
            /* If we failed to convert, maybe it is an option */
            if ( strcmp("--help", argv[i]) == 0 )
            {
                usage();
                returnCode = 0;
                goto __cleanup_and_exit;
            }
            
            if ( strcmp("--version", argv[i]) == 0 )
            {
                fprintf(stderr, "\ngetcpids version %s by Timothy Savannah\n\n", PID_TOOLS_VERSION);
                returnCode = 0;
                goto __cleanup_and_exit;
            }

            if ( argv[i][0] == '-' && (argv[i][1] == 'r' || argv[i][1] == 'R') && argv[i][2] == '\0' )
            {
                isRecursiveMode = 1;
                continue;
            }

            /* Nope -- fail out. */
            fprintf(stderr, "Invalid pid: %s\n", argv[i]);
            returnCode = 1;
            goto __cleanup_and_exit;
        }

        providedPids[ numPidArgs++ ] = providedPid;
    }


    numItems = 0;

    matchedPidsMap = simple_int_map_create(10);
    /* First, assemble all pids into a single map (so we aren't opendir/readdir/closedir
     *    over and over with conflicts in static data in recursive mode )
     */

    allPidsMap = simple_int_map_create(25);
    /* Iterate over entries in /proc looking for numeric folders.
     *   These are active pids.
     *   Directory info is returned already-sorted, so no need to sort output
     */
    procDir = opendir("/proc");
    while( (dirInfo = readdir(procDir)) )
    {
        nextPidStr = dirInfo->d_name;
        if(!isdigit(nextPidStr[0]))
                continue;

        nextPid = atoi(nextPidStr);

        simple_int_map_add( allPidsMap, nextPid );
    }
    closedir(procDir);

    allPids = simple_int_map_values(allPidsMap, &allPidsLen);

    for( i=0; i < allPidsLen; i++ )
    {
        nextPid = allPids[i];

        if ( i > 0 && simple_int_map_contains( matchedPidsMap, nextPid ) )
            continue;

        ppid = getPpid(nextPid);

        /* Iterate over each argument and if parent pid of
         *   entryy we are checking is a match, we add to list.
         * Duplicates are handled by break-ing after a match.
         */
        for( j=0; j < numPidArgs; j++ )
        {
            providedPid = providedPids[j];
            if(ppid == providedPid)
            {
                /* Is recursive, so add this pid and recurse */
                simple_int_map_add( matchedPidsMap, nextPid );

                if ( isRecursiveMode == 0 )
                {
                    break;
                }
                else
                {
                    get_cpids_recursive( matchedPidsMap, nextPid , allPids, allPidsLen);
                }
            }
        }
    }

    numItems = MAP_NUM_ENTRIES(matchedPidsMap);
    /* Check for no matched children. */
    if ( numItems == 0 )
        goto __cleanup_and_exit;

    printList = simple_int_map_values(matchedPidsMap, &numItems);

    qsort(printList, numItems, sizeof(int), cmp_pids);

    for( i=0; i < numItems; i++ )
    {
        printf("%d", printList[i]);
        if ( i + 1 < numItems )
            putchar(' ');
    }
    putchar('\n');
    free(printList);

__cleanup_and_exit:

    if ( providedPids != NULL )
        free(providedPids);

    if ( matchedPidsMap != NULL )
        simple_int_map_destroy(matchedPidsMap);

    if ( allPidsMap != NULL )
        simple_int_map_destroy(allPidsMap);

    if ( allPids != NULL )
        free(allPids);

    return returnCode;
}
