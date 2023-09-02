#ifndef AF_UNIX_H
#define AF_UNIX_H

#include <linuxmt/un.h>

#define NSOCKETS_UNIX 16

#define last_unix_data (unix_datas + NSOCKETS_UNIX - 1)
#define UN_DATA(SOCK) ((struct unix_proto_data *)(SOCK)->data)
#define UN_BUF_SIZE 64

#define UN_BUF_AVAIL(UPD)	(((UPD)->bp_head - (UPD)->bp_tail) & \
							(UN_BUF_SIZE-1))
#define UN_BUF_SPACE(UPD)	((UN_BUF_SIZE-1) - UN_BUF_AVAIL(UPD))

struct unix_proto_data {
    int refcnt;
    struct socket *socket;
    struct sockaddr_un sockaddr_un;
    short sockaddr_len;
    char buf[UN_BUF_SIZE];
    int bp_head, bp_tail;
    struct inode *inode;
    struct unix_proto_data *peerupd;
    struct wait_queue wait;
    int lock_flag;
    sem_t sem;
};

#endif
