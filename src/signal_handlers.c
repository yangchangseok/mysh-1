#define _POSIX_SOURCE

#include "signal_handlers.h"
#include "commands.h"

#include <signal.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "commands.h"

void catch_sigint(int signalNo)
{
    /*
       s_bgChildPid -> pid of child process what executed with '&'

       and if fg is called while, above process is running
       s_isForeground set as 1

       ctrl + c only invoked at this circumstance
    */
    if (s_bgChildPid != -1 && (s_isForeground == 1)) {
        pid_t pid = waitpid(s_bgChildPid, NULL, WNOHANG);

        if (pid == 0) {
            kill(s_bgChildPid, SIGINT);
            s_bgChildPid = -1;
        }
    }
}

void catch_sigtstp(int signalNo)
{
  // TODO: File this!
}

void catch_sigchld(int signalNo)
{
    int status = 0;
    pid_t pid = 0;

    /*
        The first parameter of waitpid
        < -1   meaning wait for any child process whose process group ID is equal to the absolute value of pid.
        -1     meaning wait for any child process.
        0      meaning wait for any child process whose process group ID is equal to that of the calling process.
        > 0    meaning wait for the child whose process ID is equal to the value of pid.

        wait any child process
        this handler only registered when command using &
    */
    if (s_bgChildPid != -1) {
        pid = waitpid(s_bgChildPid, &status, WNOHANG);

        /*
            when background process running,
            if other process send sigchld
            we ignore and re-register this handler
        */
        if (pid == 0) {
            signal(SIGCHLD, catch_sigchld);
            return;
        }

        /*
           when background process terminated
           print done message and set s_bgChildPid = -1

           that is, no process is running at background
        */

        if (pid == s_bgChildPid) {
            s_bgChildPid = -1;
            printf("Background-Done %d\n", pid);
            return;
        }
    }
}


