/*
 * net/ipv4/af_inet.c
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 *
 * The kernel side part of the ELKS TCP/IP stack. It uses tcpdev.c
 * to communicate with the actual TCP/IP stack that resides in
 * user space (ktcp)..
 */

#include <linuxmt/errno.h>
#include <linuxmt/config.h>
#include <linuxmt/socket.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/stat.h>
#include <linuxmt/debug.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/net.h>
#include <linuxmt/tcpdev.h>

#include "af_inet.h"

#ifdef CONFIG_INET

extern char tdin_buf[];
extern short bufin_sem, bufout_sem;
extern int tcpdev_inetwrite();

int inet_process_tcpdev(register char *buf, int len)
{
    register struct socket *sock;

    sock = ((struct tdb_return_data *)buf)->sock;

    switch (((struct tdb_return_data *)buf)->type) {
    case TDT_CHG_STATE:
        sock->state = (unsigned char) ((struct tdb_return_data *)buf)->ret_value;
        tcpdev_clear_data_avail();
        break;
    case TDT_AVAIL_DATA:
        down(&sock->sem);
        sock->avail_data = ((struct tdb_return_data *)buf)->ret_value;
        up(&sock->sem);
        tcpdev_clear_data_avail();
    default:
        wake_up(sock->wait);
    }

    return 1;
}

extern char tcpdev_inuse;

static int inet_create(struct socket *sock, int protocol)
{
    debug1("inet_create(sock: 0x%x)\n", sock);

    if (protocol != 0 || !tcpdev_inuse)
        return -EINVAL;

    return 0;
}

static int inet_dup(struct socket *newsock, struct socket *oldsock)
{
    debug("inet_dup()\n");
    return inet_create(newsock, 0);
}

static int inet_release(struct socket *sock, struct socket *peer)
{
    struct tdb_release cmd;
    int ret;

    debug1("inet_release(sock: 0x%x)\n", sock);
    cmd.cmd = TDC_RELEASE;
    cmd.sock = sock;
    ret = tcpdev_inetwrite(&cmd, sizeof(struct tdb_release));
    if(ret >= 0)
	ret = 0;
    return ret;
}

static int inet_bind(register struct socket *sock, struct sockaddr *addr,
		     size_t sockaddr_len)
{
    struct tdb_bind cmd;
    int ret;

    debug1("inet_bind(sock: 0x%x)\n", sock);
    if (!sockaddr_len || sockaddr_len > sizeof(struct sockaddr_in))
        return -EINVAL;

    /* TODO : Check if the user has permision to bind the port */

    cmd.cmd = TDC_BIND;
    cmd.sock = sock;
    memcpy(&cmd.addr, addr, sockaddr_len);

    tcpdev_inetwrite(&cmd, sizeof(struct tdb_bind));

    /* Sleep until tcpdev has news */
    while (bufin_sem == 0)
        interruptible_sleep_on(sock->wait);

    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();
    if(ret >= 0)
	ret = 0;
    return ret;
}

static int inet_connect(register struct socket *sock,
			register struct sockaddr *uservaddr,
			size_t sockaddr_len, int flags)
{
    struct tdb_connect cmd;
    int ret;

    debug1("inet_connect(sock: 0x%x)\n", sock);

    if (!sockaddr_len || sockaddr_len > sizeof(struct sockaddr_in))
        return -EINVAL;

    if (((struct sockaddr_in *)uservaddr)->sin_family != AF_INET)
        return -EINVAL;

    if (sock->state == SS_CONNECTING)
        return -EINPROGRESS;

    if (sock->state == SS_CONNECTED)
        return -EISCONN;

    cmd.cmd = TDC_CONNECT;
    cmd.sock = sock;
    memcpy(&cmd.addr, uservaddr, sockaddr_len);

    tcpdev_inetwrite(&cmd, sizeof(struct tdb_connect));

    /* Sleep until tcpdev has news */
    while (bufin_sem == 0)
        interruptible_sleep_on(sock->wait);

    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();

    if(ret >= 0) {
	sock->state = SS_CONNECTED;
	ret = 0;
    }
    return ret;
}

#ifndef CONFIG_SOCK_CLIENTONLY

static int inet_listen(register struct socket *sock, int backlog)
{
    struct tdb_listen cmd;
    int ret;

    debug1("inet_listen(socket : 0x%x)\n", sock);
    cmd.cmd = TDC_LISTEN;
    cmd.sock = sock;
    cmd.backlog = backlog;

    tcpdev_inetwrite(&cmd, sizeof(struct tdb_listen));

    /* Sleep until tcpdev has news */
    while (bufin_sem == 0) {
        interruptible_sleep_on(sock->wait);
        if (current->signal)
            return -ERESTARTSYS;
    }

    ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();

    return ret;
}

static int inet_accept(register struct socket *sock,
		       register struct socket *newsock, int flags)
{
    struct tdb_accept cmd;
    int ret;

    debug2("inet_accept(sock: 0x%x newsock: 0x%x)\n", sock, newsock);
    cmd.cmd = TDC_ACCEPT;
    cmd.sock = sock;
    cmd.newsock = newsock;
    cmd.nonblock = flags & O_NONBLOCK;

    tcpdev_inetwrite(&cmd, sizeof(struct tdb_accept));

    /* Sleep until tcpdev has news */
    while (bufin_sem == 0) {
        sock->flags |= SO_WAITDATA;
        interruptible_sleep_on(sock->wait);
        sock->flags &= ~SO_WAITDATA;
        if (current->signal) {
            return -ERESTARTSYS;
	}
    }

    ret = ((struct tdb_accept_ret *)tdin_buf)->ret_value;
    tcpdev_clear_data_avail();

    if(ret >= 0) {
	newsock->state = SS_CONNECTED;
	ret = 0;
    }
    return ret;
}

#endif

static int inet_read(register struct socket *sock, char *ubuf, int size,
		     int nonblock)
{
    register struct tdb_return_data *r;
    struct tdb_read cmd;
    int ret;

    debug3("inet_read(socket: 0x%x size:%d nonblock: %d)\n", sock, size,
	   nonblock);

    if (size > TCPDEV_MAXREAD)
	size = TCPDEV_MAXREAD;
    cmd.cmd = TDC_READ;
    cmd.sock = sock;
    cmd.size = size;
    cmd.nonblock = nonblock;
    tcpdev_inetwrite(&cmd, sizeof(struct tdb_read));

    /* Sleep until tcpdev has news and we have a lock on the buffer */
    while (bufin_sem == 0)
        interruptible_sleep_on(sock->wait);

    down(&sock->sem);

    r = (struct tdb_return_data *)tdin_buf;
    ret = r->ret_value;

    if (ret > 0) {
        memcpy_tofs(ubuf, &r->data, (size_t) ret);
        sock->avail_data = 0;
    }

    up(&sock->sem);

    tcpdev_clear_data_avail();

    return ret;
}

static int inet_write(register struct socket *sock, char *ubuf, int size,
		      int nonblock)
{
    struct tdb_write cmd;
    int ret, todo;

    debug3("inet_write(socket: 0x%x size:%d nonblock: %d)\n", sock, size,
	   nonblock);
    if (size <= 0)
        return 0;

    if (sock->state == SS_DISCONNECTING)
        return -EPIPE;

    if (sock->state != SS_CONNECTED)
        return -EINVAL;

    cmd.cmd = TDC_WRITE;
    cmd.sock = sock;
    cmd.size = size;
    cmd.nonblock = nonblock;

    todo = size;
    while (todo) {
        cmd.size = todo > TDB_WRITE_MAX ? TDB_WRITE_MAX : todo;

        memcpy_fromfs(cmd.data, ubuf + size - todo, (size_t) cmd.size);
        tcpdev_inetwrite(&cmd, sizeof(struct tdb_write));
        todo -= cmd.size;

	/* Sleep until tcpdev has news and we have a lock on the buffer */
        while (bufin_sem == 0) {
            interruptible_sleep_on(sock->wait);
	}

	ret = ((struct tdb_return_data *)tdin_buf)->ret_value;
	tcpdev_clear_data_avail();
	if (ret < 0) {
            if (ret == -ERESTARTSYS) {
                schedule();
                todo += cmd.size;
            } else
                return ret;
        }
    }

    return size;
}


static int inet_select(register struct socket *sock,
		       int sel_type, select_table * wait)
{
    debug("inet_select\n");
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

static int inet_send(struct socket *sock, void *buff, int len, int nonblock,
		     unsigned int flags)
{
    if (flags != 0)
	return -EINVAL;

    return inet_write(sock, (char *) buff, len, nonblock);
}

static int inet_recv(struct socket *sock, void *buff, int len, int nonblock,
		     unsigned int flags)
{
    if (flags != 0)
        return -EINVAL;

    return inet_read(sock, (char *) buff, len, nonblock);
}

int not_implemented(void)	/* Originally returned void */
{
    debug("not_implemented\n");
    return 0;
}

/*@-type@*/

static struct proto_ops inet_proto_ops = {
    AF_INET,
    inet_create,
    inet_dup,
    inet_release,
    inet_bind,
    inet_connect,
    not_implemented,	/* inet_socketpair */

#ifdef CONFIG_SOCK_CLIENTONLY
    NULL,
#else
    inet_accept,
#endif

    not_implemented,	/* inet_getname */
    inet_read,
    inet_write,
    inet_select,
    not_implemented,	/* inet_ioctl */

#ifdef CONFIG_SOCK_CLIENTONLY
    NULL,
#else
    inet_listen,
#endif

    inet_send,
    inet_recv,
    not_implemented,	/* inet_sendto */
    not_implemented,	/* inet_recvfrom */
    not_implemented,	/* inet_shutdown */
    not_implemented,	/* inet_setsockopt */
    not_implemented,	/* inet_getsockopt */
    not_implemented,	/* inet_fcntl */
};

/*@+type@*/

void inet_proto_init(struct net_proto *pro)
{
#ifndef CONFIG_SMALL_KERNEL
    printk("TCP/IP stack by Harry Kalogirou\n");
#endif
    sock_register(inet_proto_ops.family, &inet_proto_ops);
}

#endif
