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

/* uint64 - 8-byte unsigned integer (in both 32-bit and 64-bit mode) */
typedef unsigned long long int uint64;

/* outputUnitOptions - enum for all possible output formats */
enum outputUnitOptions {
    OUTPUT_UNITS_NONE = 0,
    OUTPUT_UNITS_BYTES,
    OUTPUT_UNITS_KILOBYTES,
    OUTPUT_UNITS_KIBIBYTES,
    OUTPUT_UNITS_MEGABYTES,
    OUTPUT_UNITS_MEBIBYTES,
    OUTPUT_UNITS_GIGABYTES,
    OUTPUT_UNITS_GIBIBYTES
};


/* struct pmem_rss_info - structure containing extracted RSS-related
 *                         memory info.
 *       (uint64s -- applicable for storing the whole-digit kB or B)
 */
struct pmem_rss_info {
    uint64 rssAnon;
    uint64 rssFile;
    uint64 rssShmem;
    uint64 vmRss;
};

/* struct pmem_rss_info_converted - structure containing converted RSS-related
 *                                   memory info.
 *       (double -- applicable for storing converted values above kB)
 */
struct pmem_rss_info_converted {
    double rssAnon;
    double rssFile;
    double rssShmem;
    double vmRss;
};


/* LABELS_OUTPUT_UNITS - Labels for the various units.
 *    index matches the enum outputUnitOptions values
 */
static const char *LABELS_OUTPUT_UNITS[] = { "", "B", "kB", "KiB", "mB", "MiB", "gB", "GiB" };

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
"         -t or --total   - Print total usage by all requested pids\n" \
"                             in addition to individual\n" \
"\n" \
"  If no mode is provided, '-r' (RSS) mode is selected.\n" \
"\n" \
"     Output Units:\n" \
"       (select one for the units to use in output)\n" \
"\n"
"         -b              - Output in bytes        (B, 8 bits)\n" \
"         -k              - Output in kilobytes    (kB, 1000 bytes) [default]\n" \
"         -K              - Output in kibibyte     (KiB/KB, 1024 bytes)\n" \
"         -m              - Output in megabytes    (mB, 1000 kB)\n" \
"         -M              - Output in mebibytes    (MiB/MB, 1024 KiB)\n" \
"         -g              - Output in gigabytes    (gB, 1000 mB)\n" \
"         -G              - Output in gibibytes    (GiB/GB, 1024 MiB)\n" \
"\n"
    , stderr);

    /* Print the version at the bottom of usage */
    print_version();
}

/* Old convert_value function pre-5.0 -- straight from extracted integer to 8-spaced string */
#if 0
/**
 *  convert_value_str - Converts an extracted value into the desired unit
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
static inline void convert_value_str(char *stringValue, unsigned long int extractedValue, enum outputUnitOptions outputUnits)
{
    static double convertedValue;

    switch(outputUnits)
    {
        case OUTPUT_UNITS_BYTES:
            convertedValue = (extractedValue * 1000.0);
            /* Will always be a whole number, so don't include decimals */
            sprintf(stringValue, "%.0f", convertedValue);
            return;
            break;
        case OUTPUT_UNITS_KILOBYTES:
            break;
        case OUTPUT_UNITS_KIBIBYTES:
            convertedValue = (extractedValue * 1000.0) / 1024.0;
            break;
        case OUTPUT_UNITS_MEGABYTES:
            convertedValue = extractedValue / 1000.0;
            break;
        case OUTPUT_UNITS_MEBIBYTES:
            convertedValue = (extractedValue * 1000.0) / (1024.0 * 1024.0);
            break;
        case OUTPUT_UNITS_GIGABYTES:
            convertedValue = extractedValue / (1000.0 * 1000.0);
            break;
        case OUTPUT_UNITS_GIBIBYTES:
            convertedValue = (extractedValue * 1000.0) / (1024.0 * 1024.0 * 1024.0);
            break;
    }
    sprintf(stringValue, "%.3f", convertedValue);
}
#endif

/**
 * convert_value - Convert an extracted value (in kB) to a desired unit
 *
 *
 *      @param extractedValue <uint64> - Extracted value (in kB)
 *
 *      @param outputUnits <enum outputUnitOptions> - Desired conversion
 *
 *
 *      @return <double> - The value converted to given output unit, represented as a double
 */
static inline double convert_value(uint64 extractedValue, enum outputUnitOptions outputUnits)
{
    double convertedValue;

    switch(outputUnits)
    {
        case OUTPUT_UNITS_BYTES:
            convertedValue = (extractedValue * 1000.0);
            break;
        case OUTPUT_UNITS_KILOBYTES:
            convertedValue = (double)extractedValue;
            break;
        case OUTPUT_UNITS_KIBIBYTES:
            convertedValue = (extractedValue * 1000.0) / 1024.0;
            break;
        case OUTPUT_UNITS_MEGABYTES:
            convertedValue = extractedValue / 1000.0;
            break;
        case OUTPUT_UNITS_MEBIBYTES:
            convertedValue = (extractedValue * 1000.0) / (1024.0 * 1024.0);
            break;
        case OUTPUT_UNITS_GIGABYTES:
            convertedValue = extractedValue / (1000.0 * 1000.0);
            break;
        case OUTPUT_UNITS_GIBIBYTES:
            convertedValue = (extractedValue * 1000.0) / (1024.0 * 1024.0 * 1024.0);
            break;
    }

    return convertedValue;
}

/**
 *  convertRssValues - Converts a struct of extracted values ( pmem_rss_info )
 *      into the desired unit and extracted struct format ( pmem_rss_info_converted )
 *
 *      @param extractedValues <pmem_rss_info *> - pointer to the extracted info
 *
 *      @param outputUnits <enum outputUnitOptions> -  Enumerated integer value which describes the desired
 *                        output units.
 *
 *
 *      @return
 *
 *           pmem_rss_info_converted structure filled with converted values
 */
static inline struct pmem_rss_info_converted convertRssValues(struct pmem_rss_info *extractedValues, enum outputUnitOptions outputUnits)
{
    struct pmem_rss_info_converted convertedValues;

    convertedValues.rssAnon  = convert_value( extractedValues->rssAnon, outputUnits );
    convertedValues.rssFile  = convert_value( extractedValues->rssFile, outputUnits );
    convertedValues.rssShmem = convert_value( extractedValues->rssShmem, outputUnits );
    convertedValues.vmRss    = convert_value( extractedValues->vmRss, outputUnits );
    
    return convertedValues;
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

/**
 * extractRssValuesFromLines - Extract RSS values from status lines
 *
 *    @param lines - /proc/$pid/status lines that have been split with split_lines
 *
 *    @param numLines - Number of lines in #lines array
 *
 *
 *    @return <struct pmem_rss_info> - RSS values in kB as extracted from status lines
 */
static struct pmem_rss_info extractRssValuesFromLines(char **lines, size_t numLines)
{
    int rssAnonIdx = 0;
    int rssFileIdx = 0;
    int rssShmemIdx = 0;
    int vmRssIdx = 0;

    int checkRss = 1;

    struct pmem_rss_info extractedValues;

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

    /* Only print the things we matched. */
    if ( rssAnonIdx )
    {
        sscanf(lines[rssAnonIdx], "RssAnon:\t    %llu kB", &(extractedValues.rssAnon) );
    }
    else
    {
        extractedValues.rssAnon = 0;
    }
    if ( rssFileIdx )
    {
        sscanf(lines[rssFileIdx], "RssFile:\t%llu kB", &(extractedValues.rssFile) );
    }
    else
    {
        extractedValues.rssFile = 0;
    }
    if ( rssShmemIdx )
    {
        sscanf(lines[rssShmemIdx], "RssShmem:\t%llu kB", &(extractedValues.rssShmem) );
    }
    else
    {
        extractedValues.rssShmem = 0;
    }
    if ( vmRssIdx )
    {
        sscanf(lines[vmRssIdx], "VmRSS:\t%llu kB", &(extractedValues.vmRss) );
    }
    else
    {
        extractedValues.vmRss = 0;
    }

    return extractedValues;
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

static inline void printTotalInfoHeader(void)
{
    puts("Total memory info for all requested pids");
    puts("----------------------------------------");
}

static inline void printProcessInfoFooter(void)
{
    puts("========================================");
}

/**
 * prorcessRssLines - Process status lines associated with the RSS format (-r)
 *
 *    @param lines - /proc/$pid/status lines that have been split with split_lines
 *
 *    @param numLines - Number of lines in #lines array
 *
 *    @param outputUnits <enum outputUnitOptions> - The desired output unit
 *
 *    @param rssInfoTotal <struct pmem_rss_info *> - If NULL, totals will be skipped.
 *                              Otherwise, the processed rss values will be added to the totals.
 *
 *
 *    @return <pmem_rss_info_converted> - The converted fields extracted from provided lines and
 *                                          converted to the requested output unit
 */
static struct pmem_rss_info_converted processRssLines(char **lines, size_t numLines, enum outputUnitOptions outputUnits, struct pmem_rss_info *rssInfoTotal)
{

    struct pmem_rss_info thisRssInfo;
    struct pmem_rss_info_converted thisRssInfoConverted;


    thisRssInfo = extractRssValuesFromLines(lines, numLines);

    if ( rssInfoTotal != NULL )
    {
        rssInfoTotal->rssAnon += thisRssInfo.rssAnon;
        rssInfoTotal->rssFile  += thisRssInfo.rssFile;
        rssInfoTotal->rssShmem += thisRssInfo.rssShmem;
        rssInfoTotal->vmRss    += thisRssInfo.vmRss;
    }

    thisRssInfoConverted = convertRssValues(&thisRssInfo, outputUnits);

    return thisRssInfoConverted;
}

/**
 * get_unit_label - Get the unit label associated with the requested unit
 *
 *      @param outputUnits - The desired output unit
 *
 *
 *      @return <const char*> - A pointer to a string representing the requested unit.
 *                               This is static data and should not be freed.
 */
static inline const char* get_unit_label(enum outputUnitOptions outputUnits)
{
    return LABELS_OUTPUT_UNITS[ (int)outputUnits ];
}


static inline void printRssInfoConverted
    (struct pmem_rss_info_converted thisRssInfoConverted, enum outputUnitOptions outputUnits, const char *unitLabel)
{

    if ( outputUnits == OUTPUT_UNITS_BYTES || outputUnits == OUTPUT_UNITS_KILOBYTES )
    {
        /* If bytes or kB, we are going to print as integers */
        printf("RssAnon:\t%8llu %s\n", (uint64) thisRssInfoConverted.rssAnon, unitLabel);
        printf("RssFile:\t%8llu %s\n", (uint64) thisRssInfoConverted.rssFile, unitLabel);
        printf("RssShmem:\t%8llu %s\n", (uint64) thisRssInfoConverted.rssShmem, unitLabel);
        printf("VmRSS:\t\t%8llu %s\n", (uint64) thisRssInfoConverted.vmRss, unitLabel);
    }
    else
    {
        /* Otherwise, we will print as a double / decimal */
        printf("RssAnon:\t%8.3F %s\n", thisRssInfoConverted.rssAnon, unitLabel);
        printf("RssFile:\t%8.3F %s\n", thisRssInfoConverted.rssFile, unitLabel);
        printf("RssShmem:\t%8.3F %s\n", thisRssInfoConverted.rssShmem, unitLabel);
        printf("VmRSS:\t\t%8.3F %s\n", thisRssInfoConverted.vmRss, unitLabel);
    }

}

static inline void printRssInfo
    (struct pmem_rss_info thisRssInfo, enum outputUnitOptions outputUnits, const char *unitLabel)
{

    /* If bytes or kB, we are going to print as integers */
    printf("RssAnon:\t%8llu %s\n", thisRssInfo.rssAnon, unitLabel);
    printf("RssFile:\t%8llu %s\n", thisRssInfo.rssFile, unitLabel);
    printf("RssShmem:\t%8llu %s\n", thisRssInfo.rssShmem, unitLabel);
    printf("VmRSS:\t\t%8llu %s\n", thisRssInfo.vmRss, unitLabel);

}


static void printRssLines(char **lines, size_t numLines, enum outputUnitOptions outputUnits, struct pmem_rss_info *rssInfoTotal)
{

    struct pmem_rss_info_converted thisRssInfoConverted;
    const char *unitLabel;

    thisRssInfoConverted = processRssLines(lines, numLines, outputUnits, rssInfoTotal);

    unitLabel = get_unit_label(outputUnits);

    printRssInfoConverted( thisRssInfoConverted, outputUnits, unitLabel);
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

    /* totalInfo - If we have the "total flag" we will allocate this.
     *               Allocated vs NULL is the difference in the api,
     *                so no need for a flag.
     */
    struct pmem_rss_info *totalInfo = NULL;

    allPids = malloc( sizeof(pid_t) * argc );

    /* _ENSURE_ONE_OUTPUT_UNIT - Ensures we have not already  defined output unit.
     *      If we have, print errror message and exit.
     */
    #define _ENSURE_ONE_OUTPUT_UNIT(_newUnit) \
        if ( unlikely( outputUnits != OUTPUT_UNITS_NONE ) ) \
        { \
            if ( unlikely( outputUnits == _newUnit ) ) \
            { \
                fprintf(stderr, "Warning: Selected output unit '%s' multiple times.\n", LABELS_OUTPUT_UNITS[_newUnit]); \
            } \
            else { \
                fprintf(stderr, "Multiple output units defined. Please pick just one.\nTried to select unit as '%s' but already defined as '%s'!\n\nRun `getpcmd --help' for usage information.\n", LABELS_OUTPUT_UNITS[_newUnit], LABELS_OUTPUT_UNITS[outputUnits]); \
                goto __cleanup_and_exit; \
            } \
        }
    /* End _ENSURE_ONE_OUTPUT_UNIT macro */

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
            else if ( strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--total") == 0 )
            {
                totalInfo = calloc(1, sizeof(struct pmem_rss_info));
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
            else if ( strlen(argv[i]) == 2 && argv[i][0] == '-' )
            {
                #define _SELECT_OUTPUT_UNIT(_newUnit) _ENSURE_ONE_OUTPUT_UNIT(_newUnit); outputUnits = _newUnit;
                switch( argv[i][1] )
                {
                    case 'b':
                        _SELECT_OUTPUT_UNIT(OUTPUT_UNITS_BYTES);
                        break;
                    case 'k':
                        _SELECT_OUTPUT_UNIT(OUTPUT_UNITS_KILOBYTES);
                        break;
                    case 'K':
                        _SELECT_OUTPUT_UNIT(OUTPUT_UNITS_KIBIBYTES);
                        break;
                    case 'm':
                        _SELECT_OUTPUT_UNIT(OUTPUT_UNITS_MEGABYTES);
                        break;
                    case 'M':
                        _SELECT_OUTPUT_UNIT(OUTPUT_UNITS_MEBIBYTES);
                        break;
                    case 'g':
                        _SELECT_OUTPUT_UNIT(OUTPUT_UNITS_GIGABYTES);
                        break;
                    case 'G':
                        _SELECT_OUTPUT_UNIT(OUTPUT_UNITS_GIBIBYTES);
                        break;
                    default:
                        fprintf(stderr, "Unknown option or invalid pid: %s\n\nRun `getpcmd --help' for usage information.\n", argv[i]);
                        goto __cleanup_and_exit;
                        break;
                }
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
            printRssLines(lines, numLines, outputUnits, totalInfo);
        }

        printProcessInfoFooter();
        if ( likely( (i + 1) != numPids ) )
            putchar('\n');
        #if SPLIT_LINES_CALC_SIZE == 1
          /* If SPLIT_LINES_CALC_SIZE is 0, we are using a static buffer
           *   so don't free it.
           */
          free(lines);
          lines = NULL;
        #endif
    }

    if ( totalInfo != NULL )
    {
        struct pmem_rss_info_converted totalInfoConverted;

        totalInfoConverted = convertRssValues( totalInfo, outputUnits );

        printProcessInfoFooter();
        putchar('\n');
        printTotalInfoHeader();

        printRssInfoConverted( totalInfoConverted, outputUnits, get_unit_label(outputUnits) );

        printProcessInfoFooter();
    }


__cleanup_and_exit:

    if ( allPids != NULL )
        free(allPids);

    if ( statContents != NULL )
        free(statContents);

    if ( lines != NULL )
        free(lines);

    if ( totalInfo != NULL )
        free(totalInfo);

    return returnCode;
}

/* vim: set ts=4 sw=4 st=4 expandtab : */
