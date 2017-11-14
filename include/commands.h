#ifndef MYSH_COMMANDS_H_
#define MYSH_COMMANDS_H_

struct single_command
{
  int argc;
  char** argv;
};

int evaluate_command(int n_commands, struct single_command (*commands)[512]);

void free_commands(int n_commands, struct single_command (*commands)[512]);

#include <stdbool.h>
static const char* const PATH[] = {
    "/usr/local/bin",
    "/usr/bin",
    "/bin",
    "/usr/sbin",
    "/sbin",
    0,  // point NULL for indicating end of path list
};
extern int s_bgChildPid;
extern int s_isForeground;
   
int customEvaluation(struct single_command* command);

#endif // MYSH_COMMANDS_H_
