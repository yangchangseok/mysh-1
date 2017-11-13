#include "utils.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void mysh_parse_command(const char* command,
                        int* n_commands,
                        struct single_command (*commands)[])
{
  char buf[4096];
  strcpy(buf, command);

  char *saveptr = NULL;
  char *tok = strtok_r(buf, "|", &saveptr);

  int ti = 0;

  while (tok != NULL) {
    struct single_command* com = *commands + ti;
    parse_single_command(tok, &com->argc, &com->argv);

    ++ti;

    tok = strtok_r(NULL, "|", &saveptr);
  }

  *n_commands = ti;
}

void parse_single_command(const char* command, int *argc, char*** argv) {
    const int kMaxArgc = 512;
    *argv = (char**)malloc(kMaxArgc * sizeof(char*));
    for (int i = 0; i < kMaxArgc; ++i)
        (*argv)[i] = NULL;

    char buf[4096];
    strcpy(buf, command);

    char *saveptr = NULL;
    char *tok = strtok_r(buf, " \n\t", &saveptr);

    int ti = 0;

    /*
        substitute '~' and tokens what start with ~/? to /home/? for implements path resolution

        substitute when
            1. token's length == 1 && token is '~'
            2. token's length >= 2 && token is start-with "~/"

        origin pwd also can take the ~ as parameter likes pwd ~
        and it will return the path of home directory

        but our built-in pwd has not been support this
        so we also discard the path resolution for pwd

        ! we don't care about the . and ..
        cause those two symbol work well likes absolute path
    */
    char tokBuff[512] = {0, };
    while (tok != NULL) {
        int tokLength = strlen(tok);
        switch (tokLength) {
            case 0:
                /*
                   unreachable, cause strtok_r's delim contains ' '
                */
                return;

            case 1:
            default:
                if (*(tok + 0) == '~') {
                    switch (tokLength) {
                        default:
                            if (*(tok + 1) != '/') {
                                sprintf(tokBuff, "%s", tok);
                                break;
                            }
                        case 1:
                            sprintf(tokBuff, "%s%s", getenv("HOME"), (tokLength == 0) ? "" : tok + 1);
                            break;
                    }
                } else {
                    sprintf(tokBuff, "%s", tok);
                }
                break;
        }
        (*argv)[ti] = (char*) malloc(strlen(tokBuff));
        strcpy((*argv)[ti], tokBuff);

        ++ti;

        tok = strtok_r(NULL, " \n\t", &saveptr);
    }

    *argc = ti;

    if (*argc == 0) {
        *argc = 1;
        (*argv)[0] = (char*)malloc(1);
        (*argv)[0][0] = '\0';
    }
}
