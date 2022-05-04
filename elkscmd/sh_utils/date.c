/*
 * date  small utility to check and set system time.
 *
 * 1999-11-07  mario.frasca@home.ict.nl
 *
 *  Copyright 1999 Mario Frasca
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)
#define errstr(str) write(STDERR_FILENO, str, strlen(str))

/* our own happy mktime() replacement, with the following drawbacks: */
/*    doesn't check boundary conditions */
/*    doesn't set wday or yday */
time_t utc_mktime(struct tm *t)
{
	static int moffset[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
	time_t ret;

  /* calculate days from years */
	ret = t->tm_year - 1970;
	ret *= 365L;

  /* count leap days in preceding years */
	ret += ((t->tm_year -1969) >> 2);

  /* calculate days from months */
	ret += moffset[t->tm_mon];

  /* add in this year's leap day, if any */
   if (((t->tm_year & 3) == 0) && (t->tm_mon > 1)) {
		ret ++;
   }

  /* add in days in this month */
   ret += (t->tm_mday - 1);

  /* convert to hours */
	ret *= 24L;  
   ret += t->tm_hour;

  /* convert to minutes */
   ret *= 60L;
   ret += t->tm_min;

  /* convert to seconds */
  	ret *= 60L;
   ret += t->tm_sec;

  /* correct for localtime */
  tzset();
  ret += timezone;

  /* return the result */
   return ret;
}

void usage()
{
	errmsg("usage: date [-s [yy]yy-m-dTh:m:s]\n");
#if 0
	errmsg(" -i         interactively set time\n");
	errmsg(" -c STRING  interactively set time if `now' is before STRING\n");
#endif
	exit(1);
}

int decodedatestring(char *datestring, struct timeval *tv)
{
	time_t systime;
	char * p;
	struct tm tm;
	
	p = strtok(datestring, "-");
	
	tm.tm_year= tm.tm_mon= tm.tm_mday= tm.tm_hour= tm.tm_min= tm.tm_sec=0;

	do{
		tm.tm_year = atoi(p);
		if (!(p = strtok(NULL, "-"))) return -1;
		tm.tm_mon = atoi(p);tm.tm_mon--;
		if (!(p = strtok(NULL, "T"))) return -1;
		tm.tm_mday = atoi(p);
		p = strtok(NULL, ":");
		if (!p) break;
		tm.tm_hour = atoi(p);
		p = strtok(NULL, ":");
		if (!p) break;
		tm.tm_min = atoi(p);
		p = strtok(NULL, "\n");
		if (!p) break;
		tm.tm_sec = atoi(p);
	}while(0);	/* only to have that 'break' */
	if (tm.tm_year<70) tm.tm_year+=2000;
	else if (tm.tm_year<100)tm.tm_year+=1900;
	else if (tm.tm_year<1970) 
		usage();
	systime = utc_mktime(&tm);
   tv->tv_sec = systime;
   tv->tv_usec = 0;

	return 0;
}

int main(int argc, char **argv)
{
	char *p;
	//char buf[32];

	time_t systime;
	time(&systime);

	if (argc==1)
	{
		p = ctime(&systime);
		write(STDOUT_FILENO, p, strlen(p));
	}
	else
	{
		struct timeval tv;
		int param = 1;

		if (argv[param][0] != '-')
			usage();
			
		switch(argv[param][1]){
#if 0
		case 'c':
			if (decodedatestring(argv[++param], &tv)) usage();
			if (systime > tv.tv_sec)
				return 0;
		case 'i':
			fputs("insert current date: ", stdout);
			fgets(buf, sizeof(buf)-1, stdin);
			if (decodedatestring(buf, &tv)) usage();
			break;
#endif
		case 's':
			if (decodedatestring(argv[++param], &tv)) usage();
			break;
		default:
			usage();
		}

      if (settimeofday (&tv, NULL) != 0)
		{
			errmsg("Unable to set time -- must be root\n");
			exit (1);
		}
/*		
 *	trailing parameters simply ignored
 *		if (++param < argc) usage(); 
 */

	}

   return 0;
}
