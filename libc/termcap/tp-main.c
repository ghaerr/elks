#include <stdio.h>
#include <stdlib.h>
#include "t.h"

#define	BUFLEN	50

int
main(int argc, char *argv[])
{
  char buf[BUFLEN];
  int args[3];

  args[0] = atoi (argv[2]);
  args[1] = atoi (argv[3]);
  args[2] = atoi (argv[4]);

  termcap_tparam1(argv[1], buf, sizeof(buf), "LEFT", "UP", args);
  printf ("%s\n", buf);

  return 0;
}
