#ifndef LX86_LINUXMT_UIO_H
#define LX86_LINUXMT_UIO_H

struct iovec {
    void *iov_base;	/* BSD uses caddr_t (same thing in effect) */
    int iov_len;
};

#define UIO_MAXIOV	16

#endif
