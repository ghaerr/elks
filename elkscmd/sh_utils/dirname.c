#include <unistd.h>
#include <string.h>
#include <libgen.h>

int
main (int argc, char *argv[])
{
  char *line;

  if (argc == 2)
  {
    line = dirname(argv[1]);
    write(STDOUT_FILENO, line, strlen(line));
    write(STDOUT_FILENO, "\n", 1);
  }
  else
  {
    write(STDERR_FILENO, "Usage: dirname [PATH]\n", 22);
    return 1;
  }

  return 0;
}
