#ifndef LX86_LINUXMT_UTIME_H
#define LX86_LINUXMT_UTIME_H

struct utimbuf {
    time_t actime;		/* Access time.  */
    time_t modtime;		/* Modification time.  */
};

#endif
