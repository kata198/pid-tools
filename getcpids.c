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

#ifdef __GNUC__
  #define likely(x)    __builtin_expect(!!(x),1)
  #define unlikely(x)  __builtin_expect(!!(x),0)
  #define __hot __attribute__((hot))
#else
  #define likely(x)   x
  #define unlikely(x) x
  #define __hot
#endif


static volatile char *version = "0.1.0";
static volatile char *copyright = "Copyright (c) 2016 Tim Savannah.";

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

#define CP_NEXT(cp) ((compound_pids *)cp->next)

static inline compound_pids* cp_init(void)
{
    compound_pids *ret;

    ret = (compound_pids *) calloc(sizeof(compound_pids), 1);
    return ret;
}

static compound_pids* cp_extend(compound_pids *toExtend)
{
    compound_pids *extension;

    extension = cp_init();
    toExtend->next = extension;

    return extension;
}

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

static pid_t getPpid(pid_t pid)
{
    char _buff[128] = { "/proc/" };
    char *buff = _buff;
    int fd;
    pid_t ret;

    sprintf(&buff[6], "%d/stat", pid);

    fd = open(buff, O_RDONLY);
    if ( fd <= 0 ) {
        return 0;
    }

    if ( read(fd, buff, 128) <= 0 ) {
        fprintf(stderr, "Error trying to read from '%s' [%d]: %s\n", buff, errno, strerror(errno));
    }
    close(fd);    
    for(unsigned int numSpaces=0; numSpaces < 3; buff = &buff[1] ) {
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


    providedPid = atoi(argv[1]);

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

/*    pid_t pid, ppid;


    printf("%lu\n", sizeof(void *));


    ppid = getPpid(pid);
    if ( ppid == 0) {
        fprintf(stderr, "Invalid pid: %u\n", pid);
        return 1;
    }

    printf("%u\n", ppid);
*/
    return 0;

}
