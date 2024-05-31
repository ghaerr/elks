#include <stdio.h>
#include <string.h>
#include <libgen.h>

/* POSIX implementation of basename(3) */

char *
basename(char *path)
{
  char *base;
  int last;

  last = strlen (path) - 1;

  if (last == -1)
    return ".";

  while (last > 0 && path[last] == '/')
    path[last--] = '\0';

  if (last == 0 && path[0] == '/')
    return path;

  base = strrchr (path, '/');

  if (base != NULL)
      return base + 1;

  return path;
}
