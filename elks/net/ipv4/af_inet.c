/*
 * net/ipv4/af_inet.c
 *
 * TCP/IP stack by Harry Kalogirou
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 * 4 Aug 20 Greg Haerr - debugged semaphores and added multiprocess support
 *
 * The kernel side part of the ELKS TCP/IP stack. It uses tcpdev.c to
 * communicate with the actual TCP/IP stack that resides in user space (ktcp).
 */

//#define DEBUG 1
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/socket.h>
#include <linuxmt/string.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/stat.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/sched.h>
#include <linuxmt/net.h>
#include <linuxmt/in.h>
#include <linuxmt/tcpdev.h>
#include <linuxmt/debug.h>
#include <linuxmt/init.h>
#include <arch/irq.h>

#include "af_inet.h"

#ifdef CONFIG_INET

extern unsigned char tdin_buf[];
extern sem_t bufin_sem, bufout_sem;
extern char tcpdev_inuse;
extern int tcpdev_inetwrite(void *data, unsigned int len);
extern char *get_tdout_buf(void);

static sem_t rwlock;    /* global inet_read/write semaphore*/

/* datagram helpers, defined below */
static int FARPROC inet_bind_kernel(struct socket *sock, struct sockaddr_in *addr);
static int FARPROC inet_autobind(struct socket *sock);
static int FARPROC inet_dgram_connect(struct socket *sock, struct sockaddr_in *addr);
static int FARPROC inet_dgram_send(struct socket *sock, char *ubuf, int size,
                           int nonblock, unsigned int flags, struct sockaddr *uaddr);
static int FARPROC inet_dgram_recv(struct socket *sock, char *ubuf, int size,
                           int nonblock, unsigned int flags, struct sockaddr *uaddr);

int FARPROC inet_process_tcpdev(register char *buf, int len)
{
    register struct socket *sock;

    sock = ((struct tdb_return_data *)buf)->sock;
    debug_net("INET(%P) process_tcpdev sock %x type %d wait %x\n",
        sock, ((struct tdb_return_data *)buf)->type, sock->wait);

    switch (((struct tdb_return_data *)buf)->type) {
    case TDT_CHG_STATE:
        sock->state = (unsigned char) ((struct tdb_return_data *)buf)->ret_value;
        tcpdev_clear_data_avail();
        debug_net("INET(%P) chg_state sock %x %d\n", sock, sock->state);
        if (sock->state == SS_DISCONNECTING) {
            sock->flags |= SF_CLOSING;
            wake_up(sock->wait);
        }
        break;

    case TDT_AVAIL_DATA:
        down(&sock->sem);
        sock->avail_data = ((struct tdb_return_data *)buf)->ret_value;
        debug_net("INET(%P) sock %x avail %u bufin %d\n",
            sock, sock->avail_data, bufin_sem);
        up(&sock->sem);
        tcpdev_clear_data_avail();
        wake_up(sock->wait);
        break;

    case TDT_CONNECT:
        down(&sock->sem);
        sock->flags |= SF_CONNECT;
        sock->retval = ((struct tdb_return_data *)buf)->ret_value;
        debug_net("INET(%P) sock %x connect %d bufin %d\n",
            sock, sock->retval, bufin_sem);
        up(&sock->sem);
        tcpdev_clear_data_avail();
        wake_up(sock->wait);
        break;

    case TDT_RETURN:
    case TDT_ACCEPT:
    case TDT_BIND:
    case TDT_RECVFROM:
        debug_net("INET(%P) retval %d bufin %d\n",
            ((struct tdb_return_data *)buf)->ret_value, bufin_sem);
        /* tcpdev_clear_data_avail() called by woken process */
        wake_up(sock->wait);
        break;
    }

    return 1;
}

static int inet_create(struct socket *sock, int protocol)
{
    debug_net("INET(%P) create sock %x\n", sock);

    if (protocol != 0)
        return -EINVAL;
    if (!tcpdev_inuse)
        return -ENETDOWN;

    return 0;
}

static int inet_dup(struct socket *newsock, struct socket *oldsock)
{
    return inet_create(newsock, 0);
}

static int inet_release(struct socket *sock, struct socket *peer)
{
    register struct tdb_release *cmd;
    int ret;

    debug_net("INET(%P) release sock %x\n", sock);
    if (!tcpdev_inuse)
        return -EINVAL;
    cmd = (struct tdb_release *)get_tdout_buf();
    cmd->cmd = TDC_RELEASE;
    cmd->sock = sock;
    cmd->reset = sock->flags & SF_RST_ON_CLOSE;
    ret = tcpdev_inetwrite(cmd, sizeof(struct tdb_release));
    return (ret >= 0 ? 0 : ret);
}

/* bind with the address already in kernel space, for inet_autobind */
static int FARPROC inet_bind_kernel(register struct socket *sock, struct sockaddr_in *addr)
{
    register struct tdb_bind *cmd;
    int ret;

    down(&rwlock);
    cmd = (struct tdb_bind *)get_tdout_buf();
    cmd->cmd = TDC_BIND;
    cmd->proto = (sock->flags & SF_DGRAM)? TDB_PROTO_DGRAM: TDB_PROTO_STREAM;
    cmd->sock = sock;
    cmd->reuse_addr = sock->flags & SF_REUSE_ADDR;
    cmd->rcv_bufsiz = sock->rcv_bufsiz;
    memcpy(&cmd->addr, addr, sizeof(struct sockaddr_in));

    tcpdev_inetwrite(cmd, sizeof(struct tdb_bind));

    /* Sleep until tcpdev has news */
    while (bufin_sem == 0)
        interruptible_sleep_on(sock->wait);

    sock->localaddr = ((struct tdb_bind_ret *)tdin_buf)->addr_ip;
    sock->localport = ((struct tdb_bind_ret *)tdin_buf)->addr_port;
    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();
    up(&rwlock);

    debug_net("INET(%P) bind returns %d\n", ret);
    return (ret >= 0 ? 0 : ret);
}

static int inet_bind(register struct socket *sock, struct sockaddr *addr,
                     size_t sockaddr_len)
{
    struct sockaddr_in kaddr;

    debug_net("INET(%P) bind sock %x\n", sock);

    if (!sockaddr_len || sockaddr_len > sizeof(struct sockaddr_in))
        return -EINVAL;

    /* TODO : Check if the user has permision to bind the port */

    memset(&kaddr, 0, sizeof(kaddr));
    memcpy_fromfs(&kaddr, addr, sockaddr_len);

    return inet_bind_kernel(sock, &kaddr);
}

static int inet_connect(struct socket *sock, struct sockaddr *uservaddr,
                        size_t sockaddr_len, int flags)
{
    register struct tdb_connect *cmd;

    debug_net("INET(%P) connect sock %x\n", sock);

    if (!sockaddr_len || sockaddr_len > sizeof(struct sockaddr_in))
        return -EINVAL;

    if (get_user(&(((struct sockaddr_in *)uservaddr)->sin_family)) != AF_INET)
        return -EINVAL;

    /* just records the peer, no packets. cannot dissolve the association
     * the bsd way as AF_INET is 0, there is no AF_UNSPEC to test for */
    if (sock->flags & SF_DGRAM) {
        struct sockaddr_in kaddr;
        int ret;

        memcpy_fromfs(&kaddr, uservaddr, sizeof(struct sockaddr_in));
        if ((ret = inet_autobind(sock)) < 0)
            return ret;
        sock->remaddr = kaddr.sin_addr.s_addr;
        sock->remport = kaddr.sin_port;
        sock->state = SS_CONNECTED;
        return inet_dgram_connect(sock, &kaddr);
    }

    if (sock->state == SS_CONNECTING)
        return -EINPROGRESS;

    sock->flags &= ~SF_CONNECT;
    cmd = (struct tdb_connect *)get_tdout_buf();
    cmd->cmd = TDC_CONNECT;
    cmd->sock = sock;
    memcpy_fromfs(&cmd->addr, uservaddr, sockaddr_len);

    tcpdev_inetwrite(cmd, sizeof(struct tdb_connect));

    do {
        interruptible_sleep_on(sock->wait);
        if (current->signal)
            return -ETIMEDOUT;
    } while (!(sock->flags & SF_CONNECT));

    if (sock->retval == 0)
        sock->state = SS_CONNECTED;
    return sock->retval;
}


static int inet_listen(register struct socket *sock, int backlog)
{
    register struct tdb_listen *cmd;
    int ret;

    debug("inet_listen(socket : 0x%x)\n", sock);
    down(&rwlock);
    cmd = (struct tdb_listen *)get_tdout_buf();
    cmd->cmd = TDC_LISTEN;
    cmd->sock = sock;
    cmd->backlog = backlog;

    tcpdev_inetwrite(cmd, sizeof(struct tdb_listen));

    /* Sleep until tcpdev has news */
    while (bufin_sem == 0)
        interruptible_sleep_on(sock->wait);

    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();
    up(&rwlock);

    return ret;
}

static int inet_accept(register struct socket *sock, struct socket *newsock, int flags)
{
    register struct tdb_accept *cmd;
    int ret;

    debug_tune("INET(%P) accept wait sock %x newsock %x\n", sock, newsock);
    cmd = (struct tdb_accept *)get_tdout_buf();
    cmd->cmd = TDC_ACCEPT;
    cmd->sock = sock;
    cmd->newsock = newsock;
    cmd->nonblock = flags & O_NONBLOCK;

    tcpdev_inetwrite(cmd, sizeof(struct tdb_accept));

    /* Sleep until tcpdev has news */
    do {        /* always sleep once to prevent accept race condition #1082 */

        interruptible_sleep_on(sock->wait);
        //interruptible_sleep_on(newsock->wait);

        if (current->signal) {
            debug_net("INET(%P) accept RESTARTSYS bufin %d\n", bufin_sem);
            return -ERESTARTSYS;
        }
    } while (bufin_sem == 0);

    debug_tune("INET(%P) accepted sock %x newsock %x\n", sock, newsock);
    newsock->remaddr = ((struct tdb_accept_ret *)tdin_buf)->addr_ip;
    newsock->remport = ((struct tdb_accept_ret *)tdin_buf)->addr_port;
    newsock->localaddr = ((struct tdb_accept_ret *)tdin_buf)->locaddr;
    newsock->localport = ((struct tdb_accept_ret *)tdin_buf)->locport;
    ret = ((struct tdb_accept_ret *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();
    if (ret >= 0) {
        newsock->state = SS_CONNECTED;
        ret = 0;
    }
    return ret;
}

static int inet_read(struct socket *sock, char *ubuf, int size, int nonblock)
{
    register struct tdb_read *cmd;
    int ret;

    debug_net("INET(%P) read sock %x size %d nonblock %d bufin %d\n",
           sock, size, nonblock, bufin_sem);

    if (sock->flags & SF_DGRAM)         /* read() == recvfrom(NULL) */
        return inet_dgram_recv(sock, ubuf, size, nonblock, 0, NULL);

    if (size > TCPDEV_MAXREAD)
        size = TCPDEV_MAXREAD;

    /* ensure read blocks until data - wait for ktcp to report data available*/
    while (sock->avail_data == 0) {
        /* return EOF on socket remote closed*/
        if (sock->flags & SF_CLOSING)
            return 0;

        /* O_NONBLOCK was ignored here, so a nonblocking read on a quiet
         * socket slept anyway and one silent peer parked the process */
        if (nonblock)
            return -EAGAIN;

        debug_net("INET(%P) read waiting on sock->avail_data sock %x buf_in %d\n",
            sock, bufin_sem);

        interruptible_sleep_on(sock->wait);
        if (current->signal)
            return -EINTR;
    }

    down(&rwlock);
    cmd = (struct tdb_read *)get_tdout_buf();
    cmd->cmd = TDC_READ;
    cmd->sock = sock;
    cmd->size = size;
    cmd->nonblock = nonblock;
    tcpdev_inetwrite(cmd, sizeof(struct tdb_read));

    debug_net("INET(%P) read waiting on wait %x, bufin %d\n",
        sock->wait, bufin_sem);

    /* Sleep until tcpdev has news and we have a lock on the buffer */
    while (bufin_sem == 0) {
        debug_net("INET(%P) read WAIT sock %x bufin_sem\n", sock);
        interruptible_sleep_on(sock->wait);
    }
    debug_net("INET(%P) read wait done bufin_sem %d\n", bufin_sem);

    down(&sock->sem);
    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;

    if (ret > 0) {
        debug_net("INET(%P) READ %u ask %u avail %u\n",
            ret, size, sock->avail_data);

        memcpy_tofs(ubuf, &((struct tdb_return_data *)tdin_buf)->data,
            (size_t) ((struct tdb_return_data *)tdin_buf)->size);
        sock->avail_data = 0;
    } else debug_net("INET(%P) READ %d ask %u avail %u\n",
        ret, size, sock->avail_data);

    up(&sock->sem);

    tcpdev_clear_data_avail();
    up(&rwlock);
    return ret;
}

static int inet_write(register struct socket *sock, char *ubuf, int size,
                      int nonblock)
{
    register struct tdb_write *cmd;
    int ret, usize, count;

    debug("INET(%P) write sock %x size %d nonblock %d\n", sock, size, nonblock);
    if (size <= 0)
        return 0;

    if (sock->flags & SF_DGRAM)         /* write() == sendto(NULL), needs connect() */
        return inet_dgram_send(sock, ubuf, size, nonblock, 0, NULL);

    if (sock->state == SS_DISCONNECTING)
        return -EPIPE;

    if (sock->state != SS_CONNECTED)
        return -EINVAL;

    count = size;
    while (count) {
        down(&rwlock);
        cmd = (struct tdb_write *)get_tdout_buf();
        cmd->cmd = TDC_WRITE;
        cmd->sock = sock;
        cmd->nonblock = nonblock;
        cmd->size = count > TDB_WRITE_MAX ? TDB_WRITE_MAX : count;

        debug_net("INET(%P) WRITE %u\n", cmd->size);

        memcpy_fromfs(cmd->data, ubuf, (size_t) cmd->size);
        usize = cmd->size;
        tcpdev_inetwrite(cmd, sizeof(struct tdb_write));

        /* Sleep until tcpdev has news and we have a lock on the buffer */
        while (bufin_sem == 0) {
            debug_net("INET(%P) write WAIT sock %x\n", sock);
            interruptible_sleep_on(sock->wait);
        }
        debug_net("INET(%P) write WAIT done bufin_sem %d\n", bufin_sem);

        ret = ((struct tdb_return_data *)tdin_buf)->ret_value;

        debug_net("INET(%P) write retval %d\n", ret);
        tcpdev_clear_data_avail();
        up(&rwlock);

        if (ret < 0) {
            if (ret == -ERESTARTSYS) {
                /* ktcp is flow controlling us. same O_NONBLOCK bug as the
                 * read path, it span here at 10Hz instead of failing */
                if (nonblock)
                    return (count < size)? size - count: -EAGAIN;

                /* delay process 100ms*/
                current->state = TASK_INTERRUPTIBLE;
                current->timeout = jiffies() + (HZ / 10); /* 1/10 sec = 100ms*/
                schedule();
            } else
                return ret;
        }
        else {
            count -= usize;
            ubuf += usize;
        }
    }

    return size;
}


static int inet_select(register struct socket *sock, int sel_type)
{
    debug_net("INET(%P) select sock %04x wait %04x type %d avail %u\n",
         sock, sock->wait, sel_type, sock->avail_data);

    /* datagrams first, the tcp rule below would call an unconnected
     * socket permanently readable */
    if (sock->flags & SF_DGRAM) {
        if (sel_type == SEL_IN) {
            if (sock->avail_data)       /* a datagram is queued in ktcp */
                return 1;
            select_wait(sock->wait);
            return 0;
        }
        if (sel_type == SEL_OUT)
            return 1;                   /* TDC_SENDTO never queues in ktcp */
        return 0;                       /* no OOB for UDP */
    }

    if (sel_type == SEL_IN) {
        if (sock->avail_data || sock->state != SS_CONNECTED)
            return 1;
        else {
            select_wait(sock->wait);
            return 0;
        }
    } else if (sel_type == SEL_OUT)
        return 1;
    return 0;
}

/* ephemeral port before any data moves, so ktcp always has a udp_sock for
 * the handle before traffic arrives */
static int FARPROC inet_autobind(struct socket *sock)
{
    struct sockaddr_in addr;

    if (sock->localport)
        return 0;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;                  /* 0 = pick one */
    addr.sin_addr.s_addr = INADDR_ANY;
    return inet_bind_kernel(sock, &addr);
}

/* record the peer with ktcp, nothing goes on the wire */
static int FARPROC inet_dgram_connect(struct socket *sock, struct sockaddr_in *addr)
{
    register struct tdb_connect *cmd;
    int ret;

    down(&rwlock);
    cmd = (struct tdb_connect *)get_tdout_buf();
    cmd->cmd = TDC_CONNECT;
    cmd->sock = sock;
    memcpy(&cmd->addr, addr, sizeof(struct sockaddr_in));
    tcpdev_inetwrite(cmd, sizeof(struct tdb_connect));

    while (bufin_sem == 0)
        interruptible_sleep_on(sock->wait);
    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();
    up(&rwlock);
    return (ret >= 0)? 0: ret;
}

static int FARPROC inet_dgram_send(register struct socket *sock, char *ubuf, int size,
                           int nonblock, unsigned int flags, struct sockaddr *uaddr)
{
    register struct tdb_sendto *cmd;
    struct sockaddr_in dest;
    int ret;

    if (flags & ~MSG_SUPPORTED)
        return -EOPNOTSUPP;
    if (size > TDB_WRITE_MAX)           /* no IP fragmentation exists in ktcp */
        return -EMSGSIZE;

    if (uaddr) {
        memcpy_fromfs(&dest, uaddr, sizeof(struct sockaddr_in));
        if (dest.sin_family != AF_INET)
            return -EINVAL;
    } else {
        if (sock->state != SS_CONNECTED)
            return -EDESTADDRREQ;       /* write() on an unconnected datagram */
        dest.sin_addr.s_addr = sock->remaddr;
        dest.sin_port = sock->remport;
    }

    if ((ret = inet_autobind(sock)) < 0)
        return ret;

    down(&rwlock);
    cmd = (struct tdb_sendto *)get_tdout_buf();
    cmd->cmd = TDC_SENDTO;
    cmd->msgflags = (unsigned char)flags;
    cmd->sock = sock;
    cmd->daddr = dest.sin_addr.s_addr;
    cmd->dport = dest.sin_port;
    cmd->size = size;
    cmd->nonblock = nonblock;
    if (size)
        memcpy_fromfs(cmd->data, ubuf, (size_t)size);

    /* send only the header plus the payload, not the whole struct */
    tcpdev_inetwrite(cmd, (unsigned)((char *)cmd->data - (char *)cmd) + size);

    while (bufin_sem == 0)
        interruptible_sleep_on(sock->wait);

    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();
    up(&rwlock);
    return ret;
}

static int FARPROC inet_dgram_recv(register struct socket *sock, char *ubuf, int size,
                           int nonblock, unsigned int flags, struct sockaddr *uaddr)
{
    register struct tdb_recvfrom *cmd;
    struct tdb_recvfrom_ret *r;
    struct sockaddr_in from;
    int ret;

    if (flags & ~MSG_SUPPORTED)
        return -EOPNOTSUPP;
    if ((ret = inet_autobind(sock)) < 0)
        return ret;
    if (size > (int)TCPDEV_MAXDGRAM)
        size = TCPDEV_MAXDGRAM;

    for (;;) {
        /* no EOF test, a datagram socket has no end of stream */
        while (sock->avail_data == 0) {
            if (nonblock || (flags & MSG_DONTWAIT))
                return -EAGAIN;
            interruptible_sleep_on(sock->wait);
            if (current->signal)
                return -EINTR;
        }

        down(&rwlock);
        cmd = (struct tdb_recvfrom *)get_tdout_buf();
        cmd->cmd = TDC_RECVFROM;
        cmd->msgflags = (unsigned char)flags;
        cmd->sock = sock;
        cmd->size = size;
        cmd->nonblock = nonblock;
        tcpdev_inetwrite(cmd, sizeof(struct tdb_recvfrom));

        while (bufin_sem == 0)
            interruptible_sleep_on(sock->wait);

        r = (struct tdb_recvfrom_ret *)tdin_buf;
        ret = r->ret_value;
        if (ret > 0) {
            memcpy_tofs(ubuf, r->data, (size_t)r->size);
            if (uaddr) {
                from.sin_family = AF_INET;
                from.sin_addr.s_addr = r->saddr;
                from.sin_port = r->sport;
                memcpy_tofs(uaddr, &from, sizeof(from));
            }
            /* ktcp tells us what is still queued */
            down(&sock->sem);
            sock->avail_data = r->avail_next;
            up(&sock->sem);
        }
        tcpdev_clear_data_avail();
        up(&rwlock);

        /* avail_data can be stale after fork, ktcp is the authority so the
         * loser goes back to sleep */
        if (ret == -EAGAIN && !nonblock && !(flags & MSG_DONTWAIT)) {
            down(&sock->sem);
            sock->avail_data = 0;
            up(&sock->sem);
            continue;
        }
        return ret;
    }
}

static int inet_send(struct socket *sock, void *buff, int len, int nonblock,
                     unsigned int flags, struct sockaddr *uaddr)
{
    if (sock->flags & SF_DGRAM)
        return inet_dgram_send(sock, buff, len, nonblock, flags, uaddr);
    if (flags != 0 || uaddr)
        return -EOPNOTSUPP;

    return inet_write(sock, buff, len, nonblock);
}

static int inet_recv(struct socket *sock, void *buff, int len, int nonblock,
                     unsigned int flags, struct sockaddr *uaddr)
{
    if (sock->flags & SF_DGRAM)
        return inet_dgram_recv(sock, buff, len, nonblock, flags, uaddr);
    if (flags != 0 || uaddr)
        return -EOPNOTSUPP;

    return inet_read(sock, buff, len, nonblock);
}

static int inet_getname(struct socket *sock, struct sockaddr *usockaddr,
        int *usockaddr_len, int peer)
{
    struct sockaddr_in sockaddr;

    sockaddr.sin_family = AF_INET;
    if (peer) {
        if (sock->state != SS_CONNECTED)
            return -EINVAL;
        sockaddr.sin_port = sock->remport;
        sockaddr.sin_addr.s_addr = sock->remaddr;
    } else {
        sockaddr.sin_port = sock->localport;
        sockaddr.sin_addr.s_addr = sock->localaddr;
    }

    return move_addr_to_user((char *)&sockaddr, sizeof(struct sockaddr_in),
                                    (char *)usockaddr, usockaddr_len);
}

int not_implemented(void)
{
    debug("not_implemented\n");
    return 0;
}

static struct proto_ops inet_proto_ops = {
    AF_INET,
    inet_create,
    inet_dup,
    inet_release,
    inet_bind,
    inet_connect,
    not_implemented,    /* inet_socketpair */
    inet_accept,
    inet_getname,
    inet_read,
    inet_write,
    inet_select,
    not_implemented,    /* inet_ioctl */
    inet_listen,
    inet_send,
    inet_recv,
};

void inet_proto_init(struct net_proto *pro)
{
    sock_register(inet_proto_ops.family, &inet_proto_ops);
}

#endif
