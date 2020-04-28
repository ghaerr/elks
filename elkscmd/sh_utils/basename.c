#include <unistd.h>
#include <string.h>
#include <libgen.h>

static void
remove_suffix (register char *name, register char *suffix)
{
  register char *np, *sp;

  np = name + strlen (name);
  sp = suffix + strlen (suffix);

  while (np > name && sp > suffix)
    if (*--np != *--sp)
      return;

  if (np > name)
    *np = '\0';
}

int
main (int argc, char *argv[])
{
  char *line;

  if (argc < 2)
  {
    write(STDERR_FILENO, "Usage: basename [PATH] [SUFFIX]\n", 32);
    return 1;
  }

  if (argc == 2 || argc == 3)
  {
    line = basename(argv[1]);

    if (argc == 3)
      remove_suffix (line, argv[2]);

    write(STDOUT_FILENO, line, strlen(line));
    write(STDOUT_FILENO, "\n", 1);
  }

  return 0;
}
