#ifndef AF_UNIX_H
#define AF_UNIX_H

#include <linuxmt/un.h>

#define NSOCKETS_UNIX 16

#define UN_DATA(SOCK) ((struct unix_proto_data *)(SOCK)->data)
#define UN_BUF_SIZE 128      /* must be power of 2 */

#define UN_BUF_AVAIL(UPD)	(((UPD)->bp_head - (UPD)->bp_tail) & (UN_BUF_SIZE-1))
#define UN_BUF_SPACE(UPD)	((UN_BUF_SIZE-1) - UN_BUF_AVAIL(UPD))

struct unix_proto_data {
    int bp_head, bp_tail;
    int refcnt;
    int index;
    struct socket *socket;
    struct inode *inode;
    struct unix_proto_data *peerupd;
    struct sockaddr_un sockaddr_un;
    short sockaddr_len;
    sem_t sem;
    struct wait_queue wait;
    char buf[UN_BUF_SIZE];
};

#endif
