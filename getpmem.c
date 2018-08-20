/*
 * Copyright (c) 2018 Timothy Savannah All Rights Reserved
 *
 * Licensed under terms of Gnu General Public License Version 2
 *
 * See "LICENSE" with the source distribution for details.
 *
 * getpmem.c - "main" for "getpmem" application -
 *  Gets memory info for one or more pids
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
#include <inttypes.h>

#include "pid_tools.h"
#include "pid_utils.h"

#define OUTPUT_MODE_RSS 1

/* outputUnitOptions - enum for all possible output formats */
enum outputUnitOptions {
    OUTPUT_UNITS_NONE = 0,
    OUTPUT_UNITS_KILOBYTES = 1,
    OUTPUT_UNITS_MEGABYTES = 2
};

/* LABELS_OUTPUT_UNITS - Labels for the various units.
 *    index matches the enum outputUnitOptions values
 */
static const char *LABELS_OUTPUT_UNITS[] = { "", "kB", "mB" };

/* The actual size as of linux 4.17.13 is around 1050 bytes.
 *   So overkill by a factor of 4. Bwahahahahaha!
 */
#define STATUS_BUFFER_SIZE 4096

const volatile char *copyright = "getpmem - Copyright (c) 2018 Tim Savannah.";

static inline void print_version(void)
{
    fprintf(stderr, "getpmem version %s by Timothy Savannah\n", PID_TOOLS_VERSION);
}

static inline void print_usage(void)
{
    fputs("Usage: getpmem (Options) [pid] (Optional: [pid2] [pid..N])\n", stderr);
    fputs("  Prints the memory usage information of one or more pids\n\n", stderr);
    fputs( \
"    Options:\n" \
"\n" \
"         --help          - Print usage information\n" \
"         --version       - Print version information on getpmem\n" \
"\n" \
"     Output Mode:\n" \
"       (select one or more of the following)\n" \
"\n" \
"         -r              - Print RSS (Resident Memory Size) info\n" \
"\n" \
"\n" \
"  If no mode is provided, '-r' (RSS) mode is selected.\n" \
"\n" \
"     Output Units:\n" \
"       (select one for the units to use in output)\n" \
"\n"
"         -k              - Output in kilobytes (kB, 1000 bytes) [default]\n" \
"         -m              - Output in megabytes (mB, 1000 kB)\n" \
"\n"
    , stderr);

    /* Print the version at the bottom of usage */
    print_version();
}

/**
 *  convert_value - Converts an extracted value into the desired unit
 *                    and as a string.
 *
 *                   Converting to a string allowed lining up the output
 *
 *      stringValue - A pointer to allocated memory which will hold the result
 *
 *      extractedValue - The kB value extracted from the "status" file
 *
 *      outputUnits -  Enumerated integer value which describes the desired
 *                        output units.
 */
static inline void convert_value(char *stringValue, unsigned long int extractedValue, enum outputUnitOptions outputUnits)
{
    static double convertedValue;

    switch(outputUnits)
    {
        case OUTPUT_UNITS_MEGABYTES:
            convertedValue = extractedValue / 1000.0;
            break;
    }
    sprintf(stringValue, "%.3f", convertedValue);
}


/**
 * read_status_contents - Reads the contents of /proc/$pid/status
 *   and places the data within the memory pointed to by #buffer
 *
 *     pid   -  The process ID
 *
 *     buffer - A pointer to an allocated char* in which to place the contents
 *
 *  Return Value:
 *
 *      Returns the number of bytes read.
 *      Returns 0 on error.
 */
static size_t read_status_contents(pid_t pid, char **buffer)
{
    FILE *statFile;
    static char procPath[24] = { '/', 'p', 'r', 'o', 'c', '/' };
    char *buf;
    size_t numBytesRead;

    numBytesRead = sprintf( &procPath[6], "%u/status", pid);

    buf = *buffer;

    statFile = fopen(procPath, "r");
    if( !statFile )
        return 0;

    numBytesRead = fread(buf, 1, STATUS_BUFFER_SIZE, statFile);
    buf[numBytesRead] = '\0';

    fclose(statFile);

    return numBytesRead;
}

/* split_lines function can either assume a large number of lines,
 *  or it will have to iterate over the entire input contents twice,
 *   once to calculate number of lines and another to actually perform
 *   the operation.
 *
 * Set SPLIT_LINES_CALC_SIZE to 1 to enable the double-iteration count method.
 */
#ifndef SPLIT_LINES_CALC_SIZE

#define SPLIT_LINES_CALC_SIZE 0
/*#define SPLIT_LINES_CALC_SIZE 1*/

#endif

#define SPLIT_LINES_MAX_LINES 100

/**
 * split_lines - Splits the lines in #inputStr and returns
 *      an array of character pointers, each one pointing to
 *      the start of a line.
 *
 *      Will replace the newline characters with a null
 *        character.
 *
 *      The returned list is indexes within #inputStr,
 *        not copies, and it gets modified.
 *
 *    inputStr - String to split by newlines
 *
 *    _numLines - Pointer to a size_t which will be set
 *         to the number of lines.
 *
 *  Return:
 *
 *    A char** where the values are the beginnings of lines
 *      within #inputStr.
 *
 *    If SPLIT_LINES_CALC_SIZE is 0, this is a static array and
 *      should not be freed. Otherwise, it is dynamic and must be freed.
 */
static char **split_lines(char *inputStr, size_t *_numLines)
{
    #if SPLIT_LINES_CALC_SIZE == 0
      static char **ret = NULL;
    #else
      char **ret;
    #endif
    int retIdx = 0;

    char *cur;

    size_t numLines;


    #if SPLIT_LINES_CALC_SIZE == 0
      /* 
       * If we are going to just use one large buffer, make it static
       *   and allocate it once.
       */
      if ( ret == NULL )
          ret = malloc( sizeof(char *) * SPLIT_LINES_MAX_LINES );
    #else
      numLines = 0;
      for( cur=inputStr; *cur != '\0'; cur++ )
      {
          if ( *cur == '\n' )
              numLines += 1;
      }

      ret = malloc( sizeof(char *) * numLines);
    #endif


    /* Assign our first line at the string start */
    ret[ retIdx++ ] = inputStr;
    for( cur=inputStr; *cur != '\0'; cur++ )
    {
        if ( *cur == '\n' )
        {
            /* 
             * Mark the next pointer following the newline
             *  and replace the newline with a null to end
             *  previous string. 
             */

             /* Make sure we aren't at the end */
             if ( unlikely(cur[1] == '\0') )
             {
                /* Go ahead and exit */
                break;
             }
            *cur = '\0';

            cur += 1;
            ret[ retIdx++ ] = cur;
        }
    }
    #if SPLIT_LINES_CALC_SIZE == 0
      numLines = retIdx;
    #endif

    *_numLines = numLines;

    return ret;
}


static inline void printProcessInfoHeader(pid_t curPid, char **lines, size_t numLines)
{
    static const char *UNKNOWN_NAME = "UNKNOWN";
    char *namePtr = NULL;

    if ( lines != NULL )
    {
        int i;
        char *curLine;

        static const uint32_t NAME_STR = ('e' << 24) + ('m' << 16) + ('a' << 8) + 'N';

        for( i=0; i < numLines; i++ )
        {
            curLine = lines[i];
            if ( ((uint32_t *)curLine)[0] == NAME_STR && curLine[4] == ':' )
            {
                namePtr = &curLine[5];
                while( *namePtr == '\t' || *namePtr == ' ' )
                    namePtr += 1;
                break;
            }
        }
    }

    if ( unlikely( namePtr == NULL ) )
        namePtr = (char *)UNKNOWN_NAME;

    printf("Memory info for pid: %d ( %s )\n", curPid, namePtr);
    puts("----------------------------------------");
}

static inline void printProcessInfoFooter(void)
{
    puts("========================================\n");
}

/**
 * printRssLines - Print lines associated with the RSS format (-r)
 *
 *    lines - /proc/$pid/status lines that have been split with split_lines
 *
 *    numLines - Number of lines in #lines array
 *
 *  Example Output:
 *
RssAnon:            1704 kB
RssFile:            3844 kB
RssShmem:              0 kB
VmRSS:              5548 kB
 *
 */
static void printRssLines(char **lines, size_t numLines, enum outputUnitOptions outputUnits)
{
    int rssAnonIdx = 0;
    int rssFileIdx = 0;
    int rssShmemIdx = 0;
    int vmRssIdx = 0;

    int checkRss = 1;

    int i;
    char *line;

    /* Because I'm a nut and I like super-optimizing to be cool, we compress the unique
     *  portions of each string into a 32-bit unsigned integer.
     * This allows a single comparison versus 4, so 2 instructions (cmp and jne) versus 8
     */
    static const uint32_t ANON_STR = ('n' << 24) + ('o' << 16) + ('n' << 8) + 'A';
    static const uint32_t FILE_STR = ('e' << 24) + ('l' << 16) + ('i' << 8) + 'F';
    static const uint32_t SHME_STR = ('e' << 24) + ('m' << 16) + ('h' << 8) + 'S';
    static const uint32_t VMRS_STR = ('S' << 24) + ('R' << 16) + ('m' << 8) + 'V';

    uint32_t checkBlock;

    for ( i=0; i < numLines; i++ )
    {
        line = lines[i];

        if ( checkRss == 1 && strncmp("Rss", line, 3) == 0 )
        {
            /* We check the next 4 characters so gcc can optimize to a 32-bit integer
             *   and compare
             */
            checkBlock = ((uint32_t *)&line[3])[0];

            if ( checkBlock == ANON_STR )
            {
                rssAnonIdx = i;
                if ( rssAnonIdx && rssFileIdx && rssShmemIdx )
                {
                    checkRss = 0;
                    if ( vmRssIdx != 0 )
                        break;
                }

                continue;
            }
            else if ( checkBlock == FILE_STR )
            {
                rssFileIdx = i;
                if ( rssAnonIdx && rssFileIdx && rssShmemIdx )
                {
                    checkRss = 0;
                    if ( vmRssIdx != 0 )
                        break;
                }
                continue;
            }
            else if ( checkBlock == SHME_STR )
            {
                rssShmemIdx = i;
                if ( rssAnonIdx && rssFileIdx && rssShmemIdx )
                {
                    checkRss = 0;
                    if ( vmRssIdx != 0 )
                        break;
                }
                continue;
            }
            else
                continue;
        }

        else if ( vmRssIdx == 0 && ((uint32_t *)line)[0] == VMRS_STR )
        {
            vmRssIdx = i;
        }
    } /* end for loop */

    if ( outputUnits == OUTPUT_UNITS_KILOBYTES )
    {
        if ( rssAnonIdx )
            printf("%s\n", lines[rssAnonIdx]);
        if ( rssFileIdx )
            printf("%s\n", lines[rssFileIdx]);
        if ( rssShmemIdx )
            printf("%s\n", lines[rssShmemIdx]);

        if ( vmRssIdx )
        {
            line = lines[vmRssIdx];

            /* Add an extra tab to VmRssIdx to it shows up aligned. */
            for(i=0; line[i] != '\0'; i++ )
            {
                putchar(line[i]);
                if ( unlikely( line[i] == '\t' ) )
                    putchar('\t');
            }
            putchar('\n');
        }
    }
    else
    {
        const char *unitLabel;
        unsigned long int extractedValue;
        static char stringValue[64];


        unitLabel = LABELS_OUTPUT_UNITS[(int)outputUnits];

        /* Only print the things we matched. */
        if ( rssAnonIdx )
        {
            sscanf(lines[rssAnonIdx], "RssAnon:\t    %lu kB", &extractedValue);
            convert_value(stringValue, extractedValue, outputUnits);
                
            printf("RssAnon:\t%8s %s\n", stringValue, unitLabel);
        }
        if ( rssFileIdx )
        {
            sscanf(lines[rssFileIdx], "RssFile:\t%lu kB", &extractedValue);
            convert_value(stringValue, extractedValue, outputUnits);

            printf("RssFile:\t%8s %s\n", stringValue, unitLabel);
        }
        if ( rssShmemIdx )
        {
            sscanf(lines[rssShmemIdx], "RssShmem:\t%lu kB", &extractedValue);
            convert_value(stringValue, extractedValue, outputUnits);

            printf("RssShmem:\t%8s %s\n", stringValue, unitLabel);
        }
        if ( vmRssIdx )
        {
            sscanf(lines[vmRssIdx], "VmRSS:\t%lu kB", &extractedValue);
            convert_value(stringValue, extractedValue, outputUnits);

            printf("VmRSS:\t\t%8s %s\n", stringValue, unitLabel);
        }

    }

}


/**
 * main - Takes one or more requires arguments, the pid(s).
 *    May have options as well.
 *
 *
 *   Will look up memory information on the given pids
 *    and report it according to desired output mode(s).
 */
int main(int argc, char* argv[])
{
    pid_t *allPids = NULL;
    size_t numPids = 0;

    char **lines = NULL;
    size_t numLines;

    pid_t curPid;

    int returnCode = 0;
    int outputMode = 0;
    enum outputUnitOptions outputUnits = OUTPUT_UNITS_NONE;
    int i;

    char *statContents = NULL;
    size_t statContentsSize;

    allPids = malloc( sizeof(pid_t) * argc );


    for( i=1; i < argc; i++ )
    {
        /* Normally if strtoint returned 0 we would have to check errno,
         *   but since 0 is an invalid pid anyway and so are negative values,
         *   can skip clearing/checking errno and just assume > 0 is valid.
         */
        curPid = strtoint(argv[i]);
        if ( curPid > 0 )
        {
            allPids[ numPids++ ] = curPid;
        }
        else
        {
            if ( strcmp(argv[i], "-r") == 0 )
            {
                outputMode |= OUTPUT_MODE_RSS;
            }
            else if ( strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0 )
            {
                print_usage();
                goto __cleanup_and_exit;
            }
            else if ( strcmp(argv[i], "--version") == 0 )
            {
                print_version();
                goto __cleanup_and_exit;
            }
            else if ( strcmp(argv[i], "-k") == 0 )
            {
                if ( unlikely( outputUnits != OUTPUT_UNITS_NONE ) )
                {
                    fprintf(stderr, "Multiple output units defined. Please pick just one.\n\nRun `getpcmd --help' for usage information.\n");
                    goto __cleanup_and_exit;
                }

                outputUnits = OUTPUT_UNITS_KILOBYTES;
            }
            else if ( strcmp(argv[i], "-m") == 0 )
            {
                if ( unlikely( outputUnits != OUTPUT_UNITS_NONE ) )
                {
                    fprintf(stderr, "Multiple output units defined. Please pick just one.\n\nRun `getpcmd --help' for usage information.\n");
                    goto __cleanup_and_exit;
                }

                outputUnits = OUTPUT_UNITS_MEGABYTES;
            }
            else
            {
                fprintf(stderr, "Unknown option or invalid pid: %s\n\nRun `getpcmd --help' for usage information.\n", argv[i]);
                goto __cleanup_and_exit;
            }
        }
    }

    if ( numPids == 0 )
    {
        fprintf(stderr, "Missing any pids on which to report!\n\n");
        print_usage();
        goto __cleanup_and_exit;
    }

    /* If no output mode selected, default to RSS */
    if ( outputMode == 0 )
        outputMode = OUTPUT_MODE_RSS;

    /* If no output units selected, default to kilobytes */
    if ( outputUnits == OUTPUT_UNITS_NONE )
        outputUnits = OUTPUT_UNITS_KILOBYTES;

    /* statContents - Allocate a large static buffer to 
     *     store the /proc/pid/status contents 
     */
    statContents = malloc( sizeof(char) * STATUS_BUFFER_SIZE );

    putchar('\n');
    /* Alright, allPids contains our list of pids, we have the mode, let's go! */
    for( i=0; i < numPids; i++ )
    {
        curPid = allPids[i];


        errno = 0;
        statContentsSize = read_status_contents(curPid, &statContents);
        if ( statContentsSize == 0 )
        {
            printProcessInfoHeader(curPid, NULL, 0);
            fprintf(stderr, "Failed reading memory information for pid=%u.\n  Error %d: %s\n", curPid, errno, strerror(errno));
            printProcessInfoFooter();
            returnCode = ENOENT; /* error 2, No such file or directory */
            continue;
        }
        /*printf("Stat contents: [%d]\n%s\n", statContentsSize, statContents);*/

        lines = split_lines(statContents, &numLines);

        printProcessInfoHeader(curPid, lines, numLines);

        if ( !!( outputMode & OUTPUT_MODE_RSS ) )
        {
            printRssLines(lines, numLines, outputUnits);
        }

        printProcessInfoFooter();
        #if SPLIT_LINES_CALC_SIZE == 1
          /* If SPLIT_LINES_CALC_SIZE is 0, we are using a static buffer
           *   so don't free it.
           */
          free(lines);
          lines = NULL;
        #endif
    }


__cleanup_and_exit:

    if ( allPids != NULL )
        free(allPids);

    if ( statContents != NULL )
        free(statContents);

    if ( lines != NULL )
        free(lines);

    return returnCode;
}

/* vim: set ts=4 sw=4 st=4 expandtab : */
