#ifndef _BSD_TYPES_H_
#define _BSD_TYPES_H_

#include <linuxmt/types.h>

typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef off_t fpos_t;

#define sig_t           __sighandler_t 

#define __attribute__(_a)
#define __dead
#define MAXPATHLEN 128
#define NODEV 0
#define TTYHOG  1024
#define NOFILE  20

#define NO_SYSLOG

#define _PATH_SHELL "/bin/sh"
#define _PATH_CSHELL _PATH_SHELL
#define _PATH_VI "/bin/vi"
#define _PATH_MAILDIR "/var/mail"
#define _PATH_SENDMAIL "/bin/sendmail"

#endif _BSD_TYPES_H_
