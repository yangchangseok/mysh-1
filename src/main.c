#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "signal_handlers.h"
#include "commands.h"
#include "built_in.h"
#include "utils.h"

int main()
{
  char buf[8096];

  /*
    handle the SIGINT[Ctrl + C] and SIGSTP[Ctrl + Z]
  */
  signal(SIGINT, catch_sigint);
  signal(SIGTSTP, catch_sigtstp);

  while (1) {
    char* fgetsRet = fgets(buf, 8096, stdin);

    /*
        while fgets(what is provided) blocking the process
        if process get a signal from anywhere

        > fgets side
            then fgets return NULL

        > signal side
            re-establish the signal handler using SA_RESTART flag(sigaction)

        but our code has been and will be compiled by C99
        so we can't access to the 'struct sigaction'

        in conclusion, there are two work-arounds

        1.
        define at the source like this '#define _XOPEN_SOURCE'
        and add -D option to gcc command-line as '-D_XOPEN_SOURCE'

        2.
        handle fgets + signal situation manunally

        let's we take second work-around.
    */
    if (fgetsRet == NULL) {
        signal(SIGINT, catch_sigint);   // re-register SIGINT handler
        signal(SIGTSTP, catch_sigtstp); // re-register SIGTSTP handler
        continue;                       // and ignore this time loop
    }

    struct single_command commands[512];
    int n_commands = 0;
    mysh_parse_command(buf, &n_commands, &commands);

    int ret = evaluate_command(n_commands, &commands);

    free_commands(n_commands, &commands);
    n_commands = 0;

    if (ret == 1) {
      break;
    }
  }

  return 0;
}
