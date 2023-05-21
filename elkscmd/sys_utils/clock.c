#include <autoconf.h>           /* for CONFIG_ options */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <arch/irq.h>
#include <arch/io.h>

#define ELKS 1

/* V1.0
 * CMOS clock manipulation - Charles Hedrick, hedrick@cs.rutgers.edu, Apr 1992
 *
 * clock [-u] -r  - read cmos clock
 * clock [-u] -w  - write cmos clock from system time
 * clock [-u] -s  - set system time from cmos clock
 * clock [-u] -a  - set system time from cmos clock, adjust the time to
 *                  correct for systematic error, and put it back to the cmos.
 *  -u indicates cmos clock is kept in universal time
 *
 * The program is designed to run setuid, since we need to be able to
 * write the CMOS port.
 *
 * I don't know what the CMOS clock will do in 2000, so this program
 * probably won't work past the century boundary.
 *
 *********************
 * V1.1
 * Modified for clock adjustments - Rob Hooft, hooft@chem.ruu.nl, Nov 1992
 * Also moved error messages to stderr. The program now uses getopt.
 * Changed some exit codes. Made 'gcc 2.3 -Wall' happy.
 *
 * I think a small explanation of the adjustment routine should be given
 * here. The problem with my machine is that its CMOS clock is 10 seconds
 * per day slow. With this version of clock.c, and my '/etc/rc.local'
 * reading '/etc/clock -au' instead of '/etc/clock -u -s', this error
 * is automatically corrected at every boot.
 *
 * To do this job, the program reads and writes the file '/etc/adjtime'
 * to determine the correction, and to save its data. In this file are
 * three numbers:
 *
 * 1) the correction in seconds per day (So if your clock runs 5
 *    seconds per day fast, the first number should read -5.0)
 * 2) the number of seconds since 1/1/1970 the last time the program was
 *    used.
 * 3) the remaining part of a second which was leftover after the last
 *    adjustment
 *
 * Installation and use of this program:
 *
 * a) create a file '/etc/adjtime' containing as the first and only line:
 *    '0.0 0 0.0'
 * b) run 'clock -au' or 'clock -a', depending on whether your cmos is in
 *    universal or local time. This updates the second number.
 * c) set your system time using the 'date' command.
 * d) update your cmos time using 'clock -wu' or 'clock -w'
 * e) replace the first number in /etc/adjtime by your correction.
 * f) put the command 'clock -au' or 'clock -a' in your '/etc/rc.local'
 *
 * If the adjustment doesn't work for you, try contacting me by E-mail.
 *
 ******
 * V1.2
 *
 * Applied patches by Harald Koenig (koenig@nova.tat.physik.uni-tuebingen.de)
 * Patched and indented by Rob Hooft (hooft@EMBL-Heidelberg.DE)
 *
 * A free quote from a MAIL-message (with spelling corrections):
 *
 * "I found the explanation and solution for the CMOS reading 0xff problem
 *  in the 0.99pl13c (ALPHA) kernel: the RTC goes offline for a small amount
 *  of time for updating. Solution is included in the kernel source
 *  (linux/kernel/time.c)."
 *
 * "I modified clock.c to fix this problem and added an option (now default,
 *  look for USE_INLINE_ASM_IO) that I/O instructions are used as inline
 *  code and not via /dev/port (still possible via #undef ...)."
 *
 * With the new code, which is partially taken from the kernel sources,
 * the CMOS clock handling looks much more "official".
 * Thanks Harald (and Torsten for the kernel code)!
 *
 ******
 * V1.3
 * Canges from alan@spri.levels.unisa.edu.au (Alan Modra):
 * a) Fix a few typos in comments and remove reference to making
 *    clock -u a cron job.  The kernel adjusts cmos time every 11
 *    minutes - see kernel/sched.c and kernel/time.c set_rtc_mmss().
 *    This means we should really have a cron job updating
 *    /etc/adjtime every 11 mins (set last_time to the current time
 *    and not_adjusted to ???).
 * b) Swapped arguments of outb() to agree with asm/io.h macro of the
 *    same name.  Use outb() from asm/io.h as it's slightly better.
 * c) Changed CMOS_READ and CMOS_WRITE to inline functions.  Inserted
 *    cli()..sti() pairs in appropriate places to prevent possible
 *    errors, and changed ioperm() call to iopl() to allow cli.
 * d) Moved some variables around to localise them a bit.
 * e) Fixed bug with clock -ua or clock -us that cleared environment
 *    variable TZ.  This fix also cured the annoying display of bogus
 *    day of week on a number of machines. (Use mktime(), ctime()
 *    rather than asctime() )
 * f) Use settimeofday() rather than stime().  This one is important
 *    as it sets the kernel's timezone offset, which is returned by
 *    gettimeofday(), and used for display of MSDOS and OS2 file
 *    times.
 * g) faith@cs.unc.edu added -D flag for debugging
 *
 * V1.4: alan@SPRI.Levels.UniSA.Edu.Au (Alan Modra)
 *       Wed Feb  8 12:29:08 1995, fix for years > 2000.
 *       faith@cs.unc.edu added -v option to print version.
 *
 ******
 * V1.4.ELKS
 *
 * Converted by Shane Kerr <kerr@wizard.net>, 1998-01-03
 *
 * Removed the adjustment option, because it used floating
 * point math, and because I don't use it.  If necessary,
 * it can certainly be added back in.
 *
 * Deleted the commented out options for debug code, because
 * they were cluttering up this already cluttered source file.
 *
 * Switched from using mktime(), with the associated time zone
 * baggage, to the hand-crafted utc_mktime(), getting time
 * zone information from gettimeofday() rather than the
 * environment variables.
 *
 * Removed the code to set the timezone with settimeofday.
 * This should probably be added back in at some point...
 *
 * I had to remove the code that waits for the falling edge
 * of the update flag -- it never seems to actually happen!
 * And since the long code is so slow, the program just locks.
 *
 * v1.6 Work with RTC localtime as well as UTC
 */

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)
#define errstr(str) write(STDERR_FILENO, str, strlen(str))

/* Globals */
int readit = 0;
int writeit = 0;
int setit = 0;
int universal = 0;

int usage(void)
{
    errmsg("clock [-u] -r|w|s\n");
    errmsg("  r: read and print CMOS clock\n");
    errmsg("  w: write CMOS clock from system time\n");
    errmsg("  s: set system time from CMOS clock\n");
    errmsg("  u: CMOS clock is in universal time\n");
    exit(1);
}
#ifdef CONFIG_ARCH_8018X
unsigned char cmos_read(unsigned char reg)
{
  register unsigned char ret;
  clr_irq ();
  ret = inb_p (CONFIG_8018X_RTC + reg);
  set_irq ();
  return ret;
}

void cmos_write(unsigned char reg, unsigned char val)
{
  clr_irq ();
  outb_p (val, CONFIG_8018X_RTC + reg);
  set_irq ();
}
#else
unsigned char cmos_read(unsigned char reg)
{
  register unsigned char ret;
  clr_irq ();
  outb_p (reg | 0x80, 0x70);
  ret = inb_p (0x71);
  set_irq ();
  return ret;
}

void cmos_write(unsigned char reg, unsigned char val)
{
  clr_irq ();
  outb_p (reg | 0x80, 0x70);
  outb_p (val, 0x71);
  set_irq ();
}
#endif

int cmos_read_bcd (int addr)
{
  int b;
  b = cmos_read (addr);
  return (b & 15) + (b >> 4) * 10;
}

void cmos_write_bcd(int addr, int value)
{
  cmos_write (addr, ((value / 10) << 4) + value % 10);
}

#ifdef CONFIG_ARCH_PC98
void read_calendar(unsigned int tm_seg, unsigned int tm_offset)
{
  __asm__ volatile ("mov %0,%%es;"
                    "mov $0,%%ah;"
                    "int $0x1C;"
                    :
                    :"a" (tm_seg), "b" (tm_offset)
                    :"%es", "memory", "cc");

}

void write_calendar(unsigned int tm_seg, unsigned int tm_offset)
{
  __asm__ volatile ("mov %0,%%es;"
                    "mov $1,%%ah;"
                    "int $0x1C;"
                    :
                    :"a" (tm_seg), "b" (tm_offset)
                    :"%es", "memory", "cc");

}

int bcd_hex(unsigned char bcd_data)
{
  return (bcd_data & 15) + (bcd_data >> 4) * 10;
}

int hex_bcd(int hex_data)
{
  return ((hex_data / 10) << 4) + hex_data % 10;
}
#endif

/* our own happy mktime() replacement, with the following drawbacks: */
/*    doesn't check boundary conditions */
/*    doesn't set wday or yday */
/*    doesn't return the local time */
time_t utc_mktime(struct tm *t)
{
    static int mday[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int year_ofs;
    int i;
    time_t ret;

  /* calculate seconds from years */
    year_ofs = t->tm_year - 70;
    ret = year_ofs * (365L * 24L * 60L * 60L);

  /* calculate seconds from leap days */
    ret += ((year_ofs + 1) >> 2) * (24L * 60L * 60L);

  /* caluclate seconds from months */
    for (i=0; i<t->tm_mon; i++) {
	ret += mday[i] * (24L * 60L * 60L);
    }

  /* add in this year's leap day, if any */
    if (((t->tm_year & 3) == 0) && (t->tm_mon > 1)) {
	ret += (24L * 60L * 60L);
    }

  /* calculate seconds from days in this month */
    ret += (t->tm_mday - 1) * (24L * 60L * 60L);

  /* calculate seconds from hours today */
    ret += t->tm_hour * (60L * 60L);

  /* calculate seconds from minutes this hour */
    ret += t->tm_min * 60L;

  /* finally, add in seconds */
    ret += t->tm_sec;

  /* return the result */
    return ret;
}

int main(int argc, char **argv)
{
  struct tm tm;
  time_t systime;
  int arg;
  unsigned char save_control, save_freq_select;

#ifdef CONFIG_ARCH_PC98
  unsigned char timebuf[6];
  unsigned char __far *timeaddr;
  unsigned int tm_seg;
  unsigned int tm_offset;

  timeaddr = (unsigned char __far *) timebuf;
  tm_seg =  ((long) timeaddr) >> 16;
  tm_offset = ((long) timeaddr) & 0xFFFF;
#endif

  while ((arg = getopt (argc, argv, "rwsuv")) != -1)
    {
      switch (arg)
	{
	case 'r':
	  readit = 1;
	  break;
	case 'w':
	  writeit = 1;
	  break;
	case 's':
	  setit = 1;
	  break;
	case 'u':
	  universal = 1;
	  break;
	default:
	  usage ();
	}
    }

  if (readit + writeit + setit > 1)
    usage ();			/* only allow one of these */

  if (!(readit | writeit | setit ))	/* default to read */
    readit = 1;

  if (readit || setit)
    {

#ifdef CONFIG_ARCH_PC98
      read_calendar(tm_seg, tm_offset);
      tm.tm_sec = bcd_hex(timebuf[5]);
      tm.tm_min = bcd_hex(timebuf[4]);
      tm.tm_hour = bcd_hex(timebuf[3]);
      tm.tm_wday = bcd_hex(timebuf[1] & 0xF);
      tm.tm_mday = bcd_hex(timebuf[2]);
      tm.tm_mon = timebuf[1] >> 4;
      tm.tm_year = bcd_hex(timebuf[0]);
#else

/* The purpose of the "do" loop is called "low-risk programming" */
/* In theory it should never run more than once */
      do
	{
	  tm.tm_sec = cmos_read_bcd (0);
	  tm.tm_min = cmos_read_bcd (2);
	  tm.tm_hour = cmos_read_bcd (4);
	  tm.tm_wday = cmos_read_bcd (6);
	  tm.tm_mday = cmos_read_bcd (7);
	  tm.tm_mon = cmos_read_bcd (8);
	  tm.tm_year = cmos_read_bcd (9);
	}
      while (tm.tm_sec != cmos_read_bcd (0));
#endif
      if (tm.tm_year < 70)
	    tm.tm_year += 100;  /* 70..99 => 1970..1999, 0..69 => 2000..2069 */
      tm.tm_mon--;		/* DOS uses 1 base */
#ifndef CONFIG_ARCH_PC98
      tm.tm_wday -= 3;		/* DOS uses 3 - 9 for week days */
#endif
      tm.tm_isdst = -1;		/* don't know whether it's daylight */
    }

  if (readit || setit)
    {
/*
 * utc_mktime() assumes we're in Greenwich, England.  If the CMOS
 * clock isn't in GMT, we need to adjust.
 */
      systime = utc_mktime(&tm);
      if (!universal) {
#if ELKS
          tzset();			/* read TZ= env string and set timezone var */
          systime += timezone;
#else
          struct timezone tz;
          gettimeofday(NULL, &tz);
          systime += tz.tz_minuteswest * 60L;
#endif
      }
#if 0
/*
 * mktime() assumes we're giving it local time.  If the CMOS clock
 * is in GMT, we have to set up TZ so mktime knows it.  tzset() gets
 * called implicitly by the time code, but only the first time.  When
 * changing the environment variable, better call tzset() explicitly.
 */
      if (universal)
	{
	  char *zone;
	  zone = (char *) getenv ("TZ");	/* save original time zone */
	  (void) putenv ("TZ=");
	  tzset ();
	  systime = mktime (&tm);
	  /* now put back the original zone */
	  if (zone)
	    {

             char *zonebuf;
             zonebuf = malloc (strlen (zone) + 4);
             strcpy (zonebuf, "TZ=");
             strcpy (zonebuf+3, zone);
             putenv (zonebuf);
             free (zonebuf);
	    }
	  else
	    {			/* wasn't one, so clear it */
	      putenv ("TZ");
	    }
	  tzset ();
	}
      else
	{
	  systime = mktime (&tm);
	}
#endif /* 0 */
    }

  if (readit)
    {
      char *p = ctime(&systime);
      write(STDOUT_FILENO, p, strlen(p));
    }

  if (setit)
    {
      struct timeval tv;
      struct timezone tz;

/* program is designed to run setuid, be secure! */

      if (getuid () != 0)
	{
	  errmsg("Sorry, must be root to set time\n");
	  exit (2);
	}

      tv.tv_sec = systime;
      tv.tv_usec = 0;
#if ELKS
      /* system time is offset by TZ variable for now, localtime handled in C library*/
      tz.tz_minuteswest = 0;
      tz.tz_dsttime = DST_NONE;
#else
      tz.tz_minuteswest = timezone / 60;
      tz.tz_dsttime = daylight;
#endif

      if (settimeofday (&tv, &tz) != 0)
        {
	  errmsg("Unable to set time -- probably you are not root\n");
	  exit (1);
	}

    }

  if (writeit)
    {
      struct tm *tmp;
      systime = time (NULL);
      if (universal)
	tmp = gmtime (&systime);
      else
	tmp = localtime (&systime);

      clr_irq();

#ifdef CONFIG_ARCH_PC98
      timebuf[5] = hex_bcd(tmp->tm_sec);
      timebuf[4] = hex_bcd(tmp->tm_min);
      timebuf[3] = hex_bcd(tmp->tm_hour);
      timebuf[1] = hex_bcd(tmp->tm_wday);
      timebuf[2] = hex_bcd(tmp->tm_mday);
      timebuf[1] = (timebuf[1] & 0xF) + ((tmp->tm_mon + 1) << 4);
      if (tmp->tm_year >= 100)
	timebuf[0] = hex_bcd(tmp->tm_year-100);
      else
	timebuf[0] = hex_bcd(tmp->tm_year);

      write_calendar(tm_seg, tm_offset);
#else
      save_control = cmos_read (11);   /* tell the clock it's being set */
      cmos_write (11, (save_control | 0x80));
      save_freq_select = cmos_read (10);       /* stop and reset prescaler */
      cmos_write (10, (save_freq_select | 0x70));

      cmos_write_bcd (0, tmp->tm_sec);
      cmos_write_bcd (2, tmp->tm_min);
      cmos_write_bcd (4, tmp->tm_hour);
      cmos_write_bcd (6, tmp->tm_wday + 3);
      cmos_write_bcd (7, tmp->tm_mday);
      cmos_write_bcd (8, tmp->tm_mon + 1);
      cmos_write_bcd (9, tmp->tm_year);

      cmos_write (10, save_freq_select);
      cmos_write (11, save_control);
#endif
      set_irq();
    }
  return 0;
}
