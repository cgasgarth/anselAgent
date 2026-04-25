#include <mach-o/dyld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int dirname_in_place(char *path)
{
  char *slash = strrchr(path, '/');
  if(!slash)
    return -1;
  *slash = '\0';
  return 0;
}

int main(int argc, char **argv)
{
  char executable_path[PATH_MAX];
  uint32_t executable_path_size = sizeof(executable_path);

  if(_NSGetExecutablePath(executable_path, &executable_path_size) != 0)
  {
    fprintf(stderr, "Ansel launcher path is too long.\n");
    return 127;
  }

  char resolved_path[PATH_MAX];
  if(!realpath(executable_path, resolved_path))
  {
    perror("realpath");
    return 127;
  }

  if(dirname_in_place(resolved_path) != 0)
  {
    fprintf(stderr, "Unable to resolve Ansel launcher directory.\n");
    return 127;
  }

  char script_path[PATH_MAX];
  int written = snprintf(script_path, sizeof(script_path), "%s/ansel-agent-launcher.sh", resolved_path);
  if(written < 0 || (size_t)written >= sizeof(script_path))
  {
    fprintf(stderr, "Ansel launcher script path is too long.\n");
    return 127;
  }

  char **child_argv = calloc((size_t)argc + 2, sizeof(char *));
  if(!child_argv)
  {
    perror("calloc");
    return 127;
  }

  child_argv[0] = "/bin/bash";
  child_argv[1] = script_path;
  for(int i = 1; i < argc; i++)
    child_argv[i + 1] = argv[i];

  execv("/bin/bash", child_argv);
  perror("execv");
  return 127;
}
