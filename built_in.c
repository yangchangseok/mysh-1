#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>

#include "built_in.h"
#include "commands.h"

int do_cd(int argc, char** argv) {
  if (!validate_cd_argv(argc, argv))
    return -1;

  if (chdir(argv[1]) == -1)
    return -1;

  return 0;
}

int do_pwd(int argc, char** argv) {
  if (!validate_pwd_argv(argc, argv))
    return -1;

  char curdir[PATH_MAX];

  if (getcwd(curdir, PATH_MAX) == NULL)
    return -1;

  printf("%s\n", curdir);

  return 0;
}

#include <unistd.h>
#include <sys/wait.h>
int do_fg(int argc, char** argv) {
    if (!validate_fg_argv(argc, argv))
        return -1;

    if (s_bgChildPid != -1) {   // if has background child process
        printf("%d\n", s_bgChildPid);

        s_isForeground = 1;             // set foreground flag for handle SIGINT
        signal(SIGCHLD, SIG_IGN);       // ignore SIGCHLD sinal
        waitpid(s_bgChildPid, NULL, 0); // block until s_bgChildPid terminated
        s_bgChildPid = -1;              // clear background process id
    }

    return 0;
}

int validate_cd_argv(int argc, char** argv) {
  if (argc != 2) return 0;
  if (strcmp(argv[0], "cd") != 0) return 0;

  struct stat buf;
  stat(argv[1], &buf);

  if (!S_ISDIR(buf.st_mode)) return 0;

  return 1;
}

int validate_pwd_argv(int argc, char** argv) {
  if (argc != 1) return 0;
  if (strcmp(argv[0], "pwd") != 0) return 0;

  return 1;
}

int validate_fg_argv(int argc, char** argv) {
  if (argc != 1) return 0;
  if (strcmp(argv[0], "fg") != 0) return 0;
  
  return 1;
}
