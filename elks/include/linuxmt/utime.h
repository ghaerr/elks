#ifndef _LINUXMT_UTIME_H
#define _LINUXMT_UTIME_H

struct utimbuf {
	time_t actime;            /* Access time.  */
	time_t modtime;           /* Modification time.  */
};

#endif /* _LINUXMT_UTIME_H */
