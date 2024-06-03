/* Outputting a string with padding.  */

#ifdef __linux__
speed_t
#else
short
#endif
	 ospeed;

/* If OSPEED is 0, we use this as the actual baud rate.  */
char PC;

/* Actual baud rate if positive;
   - baud rate / 100 if negative.  */

static short speeds[] =
  {
#ifdef VMS
    0, 50, 75, 110, 134, 150, -3, -6, -12, -18,
    -20, -24, -36, -48, -72, -96, -192
#else /* not VMS */
    0, 50, 75, 110, 135, 150, -2, -3, -6, -12,
    -18, -24, -48, -96, -192, -384
#endif /* not VMS */
  };

void
tputs (char *str, int nlines, int (*outfun)(int))
{
  register int padcount = 0;

  if (!str)
    return;

  while (*str >= '0' && *str <= '9')
    {
      padcount += *str++ - '0';
      padcount *= 10;
    }
  if (*str == '.')
    {
      str++;
      padcount += *str++ - '0';
    }
  if (*str == '*')
    {
      str++;
      padcount *= nlines;
    }
  while (*str)
    (*outfun) (*str++);

  /* padcount is now in units of tenths of msec.  */
  padcount *= speeds[ospeed];
  padcount += 500;
  padcount /= 1000;
  if (speeds[ospeed] < 0)
    padcount = -padcount;
  else
    {
      padcount += 50;
      padcount /= 100;
    }

  while (padcount-- > 0)
    (*outfun) (PC);
}
