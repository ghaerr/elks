/*
 * date  small utility to check and set system time.
 *
 * Usage: /bin/date
 *       date [?[?]] | <date>
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
#include <sys/time.h>

/* our own happy mktime() replacement, with the following drawbacks: */
/*    doesn't check boundary conditions */
/*    doesn't set wday or yday */
/*    doesn't return the local time */
time_t utc_mktime(t)
struct tm *t;
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

  /* return the result */
   return ret;
}

void usage()
{
	fputs("date : read or modify current system date\n", stdout);
	fputs("usage: date [option] [[yy]yy-m-dTh:m:s]\n", stdout);
	fputs(" -s STRING  set time described by STRING\n", stdout);
	fputs(" -i         interactively set time\n", stdout);
	fputs(" -c STRING  interactively set time if `now' is before STRING\n", stdout);
	exit(1);
}

int decodedatestring(datestring, tv)
char * datestring;
struct timeval *tv;
{
	time_t systime;
	char * p;
	struct tm tm;
	
	p = strtok(datestring, '-');
	
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

int main(argc, argv)
char ** argv;
int argc;
{
	time_t systime;
	time(&systime);

	if (argc==1)
	{
		fputs(ctime(&systime), stdout);
	}
	else
	{
		char *p, buf[32];
		struct timeval tv;
		int param = 1;

		if (argv[param][0] != '-')
			usage();
			
		switch(argv[param][1]){
		case 'c':
			if (decodedatestring(argv[++param], &tv)) usage();
			if (systime > tv.tv_sec)
			return 0;
		case 'i':
			fputs("insert current date: ", stdout);
			fgets(buf, 31, stdin);
			if (decodedatestring(buf, &tv)) usage();
			break;

		case 's':
			if (decodedatestring(argv[++param], &tv)) usage();
			break;

		default:
			usage();
		}

      if (settimeofday (&tv, NULL) != 0)
		{
			fprintf (stderr,
				"Unable to set time -- probably you are not root\n");
			exit (1);
		}
/*		
 *	trailing parameters simply ignored
 *		if (++param < argc) usage(); 
 */

	}

   return 0;
}
