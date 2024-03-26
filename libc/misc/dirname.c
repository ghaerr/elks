#include <stdio.h>
#include <string.h>
#include <libgen.h>

/* POSIX implementation of dirname(3) */

char *
dirname(char *path)
{
  char *dir;
  int last;

  last = strlen (path) - 1;

  if (last == 0 && path[0] == '/')
    return path;

  while (last > 0 && path[last] == '/')
    path[last--] = '\0';

  dir = strrchr (path, '/');

  if (dir == NULL)
  {
    path[0] = '.';
    path[1] = '\0';
    return path;
  }

  while (dir > path && *dir == '/')
    --dir;

  dir[1] = '\0';

  return path;
}
