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

int inet_process_tcpdev(char *buf, int len)
{
    register struct tdb_return_data *r;
    register struct socket *sock;

    r = (struct tdb_return_data *) buf;

    sock = (struct socket *) r->sock;

    switch (r->type) {
    case TDT_CHG_STATE:
	sock->state = (unsigned char) r->ret_value;
	tcpdev_clear_data_avail();
	break;
    case TDT_AVAIL_DATA:
	down(&sock->sem);
	sock->avail_data = r->ret_value;
	up(&sock->sem);
	tcpdev_clear_data_avail();
    default:
	wake_up(sock->wait);
    }

    return 1;
}

static int inet_create(struct socket *sock, int protocol)
{
    debug1("inet_create(sock: 0x%x)\n", sock);

    if (protocol != 0)
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
    if (ret < 0)
	return ret;
    return 0;
}

static int inet_bind(register struct socket *sock, struct sockaddr *addr,
		     size_t sockaddr_len)
{
    struct tdb_return_data *ret_data;
    struct tdb_bind cmd;
    int ret;

    debug1("inet_bind(sock: 0x%x)\n", sock);
    if (sockaddr_len <= 0 || sockaddr_len > sizeof(struct sockaddr_in))
	return -EINVAL;

    /* TODO : Check if the user has permision to bind the port */

    cmd.cmd = TDC_BIND;
    cmd.sock = sock;
    memcpy(&cmd.addr, addr, sockaddr_len);

    tcpdev_inetwrite(&cmd, sizeof(struct tdb_bind));

    /* Sleep until tcpdev has news */
    while (bufin_sem == 0)
	interruptible_sleep_on(sock->wait);

    ret_data = tdin_buf;
    ret = ret_data->ret_value;
    tcpdev_clear_data_avail();
    if (ret < 0)
	return ret;

    return 0;
}

static int inet_connect(register struct socket *sock,
			struct sockaddr *uservaddr,
			size_t sockaddr_len, int flags)
{
    struct sockaddr_in *sockin;
    register struct tdb_return_data *r;
    struct tdb_connect cmd;
    int ret;

    debug1("inet_connect(sock: 0x%x)\n", sock);

    if (sockaddr_len <= 0 || sockaddr_len > sizeof(struct sockaddr_in))
	return -EINVAL;

    sockin = uservaddr;
    if (sockin->sin_family != AF_INET)
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

    r = tdin_buf;
    ret = r->ret_value;
    tcpdev_clear_data_avail();

    if (ret < 0) {
	return ret;
    }

    sock->state = SS_CONNECTED;
    return 0;
}

void inet_socketpair(void)
{
    debug("inet_sockpair\n");
}

#ifndef CONFIG_SOCK_CLIENTONLY

static int inet_listen(register struct socket *sock, int backlog)
{
    register struct tdb_return_data *ret_data;
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

    ret_data = tdin_buf;
    ret = ret_data->ret_value;
    tcpdev_clear_data_avail();

    return ret;
}

static int inet_accept(register struct socket *sock,
		       struct socket *newsock, int flags)
{
    struct tdb_accept cmd;
    register struct tdb_accept_ret *ret_data;
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

    ret_data = tdin_buf;
    ret = ret_data->ret_value;
    tcpdev_clear_data_avail();

    if (ret < 0)
	return ret;

    newsock->state = SS_CONNECTED;

    return 0;
}

#endif

void inet_getname(void)
{
    debug("inet_getname\n");
}

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

    r = tdin_buf;
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
    register struct tdb_return_data *r;
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

	r = tdin_buf;
	ret = r->ret_value;
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
}

void inet_ioctl(void)
{
    debug("inet_ioctl\n");
}

void inet_shutdown(void)
{
    debug("inet_shutdown\n");
}

void inet_setsockopt(void)
{
    debug("setsockopt\n");
}

void inet_getsockopt(void)
{
    debug("inet_getsockopt\n");
}

void inet_fcntl(void)
{
    debug("inet_fcntl\n");
}

void inet_sendto(void)
{
    debug("inet_sendto\n");
}

void inet_recvfrom(void)
{
    debug("inet_recvfrom\n");
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

/*@-type@*/

static struct proto_ops inet_proto_ops = {
    AF_INET,
    inet_create,
    inet_dup,
    inet_release,
    inet_bind,
    inet_connect,
    inet_socketpair,

#ifdef CONFIG_SOCK_CLIENTONLY
    NULL,
#else
    inet_accept,
#endif

    inet_getname,
    inet_read,
    inet_write,
    inet_select,
    inet_ioctl,

#ifdef CONFIG_SOCK_CLIENTONLY
    NULL,
#else
    inet_listen,
#endif

    inet_send,
    inet_recv,
    inet_sendto,
    inet_recvfrom,
    inet_shutdown,
    inet_setsockopt,
    inet_getsockopt,
    inet_fcntl,
};

/*@+type@*/

void inet_proto_init(struct net_proto *pro)
{
    printk("ELKS TCP/IP by Harry Kalogirou\n");
    sock_register(inet_proto_ops.family, &inet_proto_ops);
}

#endif
