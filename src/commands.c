#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "commands.h"
#include "built_in.h"

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
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands > 0) {
    struct single_command* com = (*commands);

    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);
    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
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
        return customEvaluation(n_commands, commands);
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
int customEvaluation(int n_commands, struct single_command (*commands)[512]) {
    struct single_command* com = (*commands);

    // Program execution by accessing absolute path
    /*
    if (customEvaluationPipe(n_commands, commands)) {
        return 0;
    }
    */

    if (customEvaluationExecution(n_commands, commands)) {
        return 0;
    }

    /*
    if (customEvaluationBackground(n_commands, commands)) {
        return 0;
    }
    */

    fprintf(stderr, "%s: command not found\n", com->argv[0]);
    return -1;
}

bool customEvaluationPipe(int n_commands, struct single_command (*commands)[512]) {
    return false;
}
bool customEvaluationExecution(int n_commands, struct single_command (*commands)[512]) {

    if (access((*commands)->argv[0], X_OK) != -1) {
        
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
        int status = 0;
        pid_t childPid  = fork();
        pid_t parentPid = getpid();
        switch (childPid) {
            case -1:
                return false;

            default:
                waitpid(childPid, &status, 0);
                return true;

            case 0:
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
                switch (execv((*commands)->argv[0], (*commands)->argv)) {
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
    } else {
        return false;
    }
    return false;
}
bool customEvaluationBackground(int n_commands, struct single_command (*commands)[512]) {
    return false;
}


















