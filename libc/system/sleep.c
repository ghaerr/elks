#ifdef L_sleep
#include <unistd.h>

#ifdef __ELKS__
/* This uses SIGALRM, it does keep the previous alarm call but will lose
 * any alarms that go off during the sleep
 */

static void
alrm(void)
{
}

unsigned int
sleep(unsigned int seconds)
{
 void (*last_alarm)();
  unsigned int prev_sec;

  prev_sec = alarm(0);
  if (prev_sec <= seconds)
   prev_sec = 1;
  else
   prev_sec -= seconds;

  last_alarm = signal(SIGALRM, alrm);
  alarm(seconds);
  pause();
  seconds = alarm(prev_sec);
  signal(SIGALRM, last_alarm);
  return seconds;
}

#else
        /* Is this a better way ? If we have select of course :-) */
unsigned int
sleep(unsigned int seconds)
{
 struct timeval timeout;
 time_t start = time((void*)0);
 timeout.tv_sec = seconds;
 timeout.tv_usec = 0;
 select(1, NULL, NULL, NULL, &timeout);
 return seconds - (time((void*)0) - start);
}
#endif

#endif
