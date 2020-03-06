#include <termcap.h>
#include "t.h"

char *
tgoto(const char *cm, int hpos, int vpos)
{
  static char *BC;
  static char *UP;
  static char tgoto_buf[50];
  int args[2];

  if (!cm)
    return NULL;

  args[0] = vpos;
  args[1] = hpos;

  return termcap_tparam1 (cm, tgoto_buf, 50, UP, BC, args);
}
