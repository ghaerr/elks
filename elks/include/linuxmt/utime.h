#ifndef __LINUXMT_UTIME_H__
#define __LINUXMT_UTIME_H__

struct utimbuf {
	time_t actime;            /* Access time.  */
	time_t modtime;           /* Modification time.  */
};

#endif /* _LINUXMT_UTIME_H */
