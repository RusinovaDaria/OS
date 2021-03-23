#include <syslog.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h> // char * getlogin ( void );
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/file.h>
#include <sys/wait.h> //wait
#include <pthread.h>
int main()
{
    syslog(LOG_WARNING, "Hello, World! \n");
    return 0;
}