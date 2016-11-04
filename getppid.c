#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


/*
 * usage - print usage/help to stderr
 */
static inline void usage()
{
    fputs("Usage: getppid [pid]\n", stderr);
    fputs("  Prints the parent process id (PPID) for a given pid.\n", stderr);
}

/*
 * getPpid - Gets the parent process ID of a provided pid.
 *
 * If no parent id is present, "1" (init) is returned. This includes
 * for pid 1 itself.
 *
 * pid - Search for parent of this pid.
 */
static pid_t getPpid(pid_t pid)
{
    char _buff[128] = { '/', 'p', 'r', 'o', 'c', '/' };
    char *buff = _buff;
    int fd;
    pid_t ret;

    sprintf(&buff[6], "%u/stat", pid);

    fd = open(buff, O_RDONLY);
    if ( fd <= 0 ) {
        return 0;
    }

    if ( read(fd, buff, 128) <= 0 ) {
        fprintf(stderr, "Error trying to read from '%s' [%d]: %s\n", buff, errno, strerror(errno));
    }
    close(fd);    
    for(unsigned int numSpaces=0; numSpaces < 3; buff = &buff[1] )
    {
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


/**
 * main - takes one argument, the search pid.
 *
 * Prints the child's parent pid to stdout. If child has no parent,
 *  1 is printed (init). This includes when "1" is queried, i.e. "ppid(1) = 1"
 *
 *  TODO: Check for --help
 */
int main(int argc, char* argv[])
{

    pid_t pid, ppid;


    if ( argc != 2 ) {
        fputs("Invalid number of arguments.\n\n", stderr);
        usage();
        return 1;
    }


    pid = atoi(argv[1]);

    ppid = getPpid(pid);
    if ( ppid == 0) {
        fprintf(stderr, "Invalid pid: %u\n", pid);
        return 1;
    }

    printf("%u\n", ppid);

    return 0;

}
