#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "commands.h"
#include "built_in.h"
#include "signal_handlers.h"

static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512]) {
    if (n_commands > 0) {
        int built_in_pos = 0;
        struct single_command* com = NULL;

        /*
           iterate all commands
        */
        for (int index = 0 ; index < n_commands ; ++index) {
            com = *commands + index;

            assert(com->argc != 0);

            built_in_pos = is_built_in_command(com->argv[0]);
            if (built_in_pos != -1) {
                if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
                    if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
                        fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
                        break;
                    }
                } else {
                    fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
                    return -1;
                }
            } else if (strcmp(com->argv[0], "") == 0) {
                return 0;
            } else if (strcmp(com->argv[0], "exit") == 0) {
                return 1;
            } else {
                /*
                   do custom job
                */
                switch (customEvaluation(com)) {
                    case -1:    return -1;
                    default:    continue;
                }
            }
        }
    }

    return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
int s_bgChildPid    = -1;
int s_isForeground  = 1;
int customEvaluation(struct single_command* command) {
    int index = 0;
    const char* path = NULL;
    char* concatenatedPath = NULL;

    /*
       search command at all PATH

       1. and make full path
       2. check executable

        if there is a executable command at path
            breaking while loop with Not-Null concatenatedPath

        else 
            breaking while loop with Null concatenatedPath
    */
    while ((path = *(PATH + index++)) != NULL) {
        concatenatedPath = (char*) malloc(strlen(path) + strlen(command->argv[0]) + 1);

        sprintf(concatenatedPath, "%s/%s", path, command->argv[0]);

        if (access(concatenatedPath, X_OK) != -1) {
            break;
        } else {
            concatenatedPath = NULL;
            free((void*) concatenatedPath);
        }
    }

    char* temp = NULL;
    if (concatenatedPath != NULL) {
        
        temp = command->argv[0];
        command->argv[0] = concatenatedPath;    // substitute command to path-concatenated (ex: /bin/cat)
        free(temp);                             // remove old command (ex: cat)

        /*
            > pid_t fork(void)
                On success, the PID of the child process is returned in the parent,
                and 0 is returned in the child.

                On failure, -1 is returned in the parent, no child process is created,
                and errno is set  appropriately.

            > pid_t waitpid(pid_t pid, int *status, int options);
                The  waitpid()  system  call  suspends  execution of the calling process
                until a child specified by pid argument has changed state.

                By default, waitpid() waits only for terminated children, but this
                behavior is modifiable via the options argument, as described below.

                waitpid(): on success, returns the process ID of the child whose state has changed;
                if WNOHANG was specified and one or more child(ren) specified by pid exist,
                but have not yet changed state, then 0 is returned.  On error, -1 is returned.
        */

        bool isBackgroundJob = false;
        if (strlen(command->argv[command->argc - 1]) == 1 &&
                command->argv[command->argc - 1][0] == '&') {
            if (s_bgChildPid != -1) {
                // we allow only one background process
                fprintf(stderr, "alreay has background process %d\n", s_bgChildPid);
                return -1;
            } else {
                isBackgroundJob = true;

                temp = command->argv[command->argc - 1];
                command->argv[command->argc - 1] = NULL;
                free(temp);         // remove & at the end of command

                command->argc -= 1; // trim size of command
            }
        } else {
            isBackgroundJob = false;
        }

        int status = 0;
        pid_t childPid  = fork();
        pid_t parentPid = getpid();
        switch (childPid) {
            case -1:
                break;

            default:
                /*
                   wait child process'es termination

                   if end of command line is &,
                   set WNOHANG option of waitpid for non-block waitpid

                   else set 0 for block waitpid
                   and we handle the termination of child at the handler for SIGCHLD
                */
                if (isBackgroundJob) {
                    if (waitpid(childPid, &status, WNOHANG) == 0) {
                        s_bgChildPid = childPid;
                        s_isForeground = 0;
                        signal(SIGCHLD, catch_sigchld);

                        printf("Background-Begin %d\n", childPid);   // print process id of child
                    } else {
                        printf("Backgournd-Done %d\n", childPid);
                    }
                } else {
                    waitpid(childPid, &status, 0);
                    /*
                        re-register signal handler
                        because, terminated with parent's signal
                        also re-establish the signal handler
                    */
                    signal(SIGINT, catch_sigint);   // re-register SIGINT handler
                    signal(SIGTSTP, catch_sigtstp); // re-register SIGTSTP handler
                }
                return 0;

            case 0:
                
                /*
                   Should ignore parent's signal
                   when child process running at background

                   so avoid parent's signal using setpgid
                */
                if (isBackgroundJob) {
                    setpgid(0, 0);
                    signal(SIGCHLD, SIG_IGN);
                    //signal(SIGINT, SIG_IGN);
                    //signal(SIGCHLD, SIG_IGN);
                }

                /*
                    > int execv(const char *path, char *const argv[]);
                        The execv() provide an array of pointers to null-terminated strings
                        that represent the argument list available to the new program.

                        The first argument,  by  convention, should point to the filename
                        associated with the file being executed.

                        The array of pointers must be terminated by a null pointer.

                        The execv() functions return only if an error has occurred.
                        The return value is -1, and errno is set to indicate the error.
                */
                switch (execv(command->argv[0], command->argv)) {
                    /*
                       unrechable, cause execv overwrite parent process

                       if code came here, it is the perfect abnormal situation
                     */
                    case -1:
                        // execv fail
                    default:
                        exit(EXIT_FAILURE);
                }
                break;
        }
    }

    fprintf(stderr, "%s: command not found\n", command->argv[0]);
    return -1;
}
