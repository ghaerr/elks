/*
 * convdate(), convtime(), convstamp()
 */
#include <stdio.h>
/*
 * convert MSDOS directory datestamp to ASCII.
 */

char *
convdate(date_high, date_low)
unsigned date_high, date_low;
{
/*
 *	    hi byte     |    low byte
 *	|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 *      | | | | | | | | | | | | | | | | |
 *      \   7 bits    /\4 bits/\ 5 bits /
 *         year +80      month     day
 */
	static char ans[9];
	unsigned char year, month_hi, month_low, day;

	year = (date_high >> 1) + 80;
	month_hi = (date_high & 0x1) << 3;
	month_low = date_low >> 5;
	day = date_low & 0x1f;
	sprintf(ans, "%2d-%02d-%02d", month_hi+month_low, day, year);
	return(ans);
}

/*
 * Convert MSDOS directory timestamp to ASCII.
 */

char *
convtime(time_high, time_low)
unsigned time_high, time_low;
{
/*
 *	    hi byte     |    low byte
 *	|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 *      | | | | | | | | | | | | | | | | |
 *      \  5 bits /\  6 bits  /\ 5 bits /
 *         hour      minutes     sec*2
 */
	static char ans[7];
	char am_pm;
	unsigned char hour, min_hi, min_low;

	hour = time_high >> 3;
	am_pm = (hour >= 12) ? 'p' : 'a';
	if (hour > 12)
		hour = hour -12;
	if (hour == 0)
		hour = 12;
	min_hi = (time_high & 0x7) << 3;
	min_low = time_low >> 5;
	sprintf(ans, "%2d:%02d%c", hour, min_hi+min_low, am_pm);
	return(ans);
}

/*
 * Convert a MSDOS time & date stamp to the Unix time() format
 */

long
convstamp(time_field, date_field)
unsigned char *time_field, *date_field;
{
	extern long timezone;
	int year, mon, mday, hour, min, sec, old_leaps;
	long answer, sec_year, sec_mon, sec_mday, sec_hour, sec_min, sec_leap;
	static int month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304,
	334};
					/* disect the parts */
	year = (date_field[1] >> 1) + 1980;
	mon = (((date_field[1] & 0x1) << 3) + (date_field[0] >> 5));
	mday = date_field[0] & 0x1f;
	hour = time_field[1] >> 3;
	min = (((time_field[1] & 0x7) << 3) + (time_field[0] >> 5));
	sec = (time_field[0] & 0x1f) * 2;
					/* how many previous leap years */
	old_leaps = (year -1972) / 4;
	sec_leap = old_leaps * 24 * 60 * 60;
					/* back off 1 day if before 29 Feb */
	if (!(year % 4) && mon < 3)
		sec_leap -= 24 * 60 * 60;
	sec_year = (year - 1970) * 365 * 24 * 60 * 60;
	sec_mon = month[mon -1] * 24 * 60 * 60;
	sec_mday = mday * 24 * 60 * 60;
	sec_hour = hour * 60 * 60;
	sec_min = min * 60;
	
	answer = sec_leap + sec_year + sec_mon + sec_mday + sec_hour + sec_min + sec/* + timezone*/;
	return(answer);
}
