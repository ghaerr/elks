/*
 * net/unix/af_unix.c
 *
 * (C) 1999 Al Riddoch <ajr@ecs.soton.ac.uk>
 *
 * A bare implentation of the features of UNIX sockets required by nano-X,
 * no more, no less.
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/socket.h>
#include <linuxmt/fs.h>
#include <linuxmt/sched.h>
#include <linuxmt/mm.h>
#include <linuxmt/stat.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>
#include <arch/irq.h>
#include "af_unix.h"

#ifdef CONFIG_UNIX

struct unix_proto_data unix_datas[NSOCKETS_UNIX];

static struct unix_proto_data *unix_data_alloc(void)
{
    struct unix_proto_data *upd;

    clr_irq();
    for (upd = unix_datas; upd <= last_unix_data; ++upd) {
	if (!upd->refcnt) {
	    /* unix domain socket not yet in itialised - bgm */
	    upd->refcnt = -1;
	    set_irq();
	    upd->socket = NULL;
	    upd->sockaddr_len = 0;
	    upd->sockaddr_un.sun_family = 0;
	    upd->bp_head = upd->bp_tail = 0;
	    upd->inode = NULL;
	    upd->peerupd = NULL;
	    upd->sem = 0;
	    return upd;
	}
    }
    set_irq();
    return NULL;
}

static struct unix_proto_data *unix_data_lookup(struct sockaddr_un *sockun,
						int sockaddr_len,
						struct inode *inode)
{
    struct unix_proto_data *upd;

    for (upd = unix_datas; upd <= last_unix_data; ++upd)
	if (upd->refcnt > 0 && upd->socket)
	    if (upd->socket->state == SS_UNCONNECTED)
		if (upd->sockaddr_un.sun_family == sockun->sun_family &&
		    upd->inode == inode)
		    return upd;
    return NULL;
}


static void unix_data_ref(struct unix_proto_data *upd)
{
    if (upd)
	++upd->refcnt;
}

static void unix_data_deref(struct unix_proto_data *upd)
{
    if (upd->refcnt == 1)
	upd->bp_head = upd->bp_tail = 0;
    --upd->refcnt;
}

static int unix_create(struct socket *sock, int protocol)
{
    struct unix_proto_data *upd;

    if (protocol != 0)
	return -EINVAL;

    if (!(upd = unix_data_alloc()))
	return -ENOMEM;

    upd->socket = sock;
    sock->data = upd;
    upd->refcnt = 1;

    return 0;
}

static int unix_dup(struct socket *newsock, struct socket *oldsock)
{
    return unix_create(newsock, 0);
}


static int unix_release(struct socket *sock, struct socket *peer)
{
    struct unix_proto_data *upd = sock->data;

    if (!upd)
	return 0;

    if (upd->socket != sock) {
	printk("UNIX: release: socket link mismatch\n");
	return -EINVAL;
    }

    if (upd->inode) {
	iput(upd->inode);
	upd->inode = NULL;
    }

    sock->data = NULL;
    upd->socket = NULL;

    if (upd->peerupd)
	unix_data_deref(upd->peerupd);

    unix_data_deref(upd);

    return 0;
}

static int unix_bind(struct socket *sock,
		     struct sockaddr *umyaddr, int sockaddr_len)
{
    struct unix_proto_data *upd = sock->data;
    unsigned short old_ds;
    int i;

    if (sockaddr_len <= 0 || sockaddr_len > sizeof(struct sockaddr_un))
	return -EINVAL;

    if (upd->sockaddr_len || upd->inode)
	return -EINVAL;		/* already bound */

    memcpy_fromfs(&upd->sockaddr_un, umyaddr, sockaddr_len);
    upd->sockaddr_un.sun_path[sockaddr_len] = '\0';

    if (upd->sockaddr_un.sun_family != AF_UNIX)
	return -EINVAL;

    old_ds = current->t_regs.ds;
    current->t_regs.ds = kernel_ds;

    i = do_mknod(upd->sockaddr_un.sun_path,
	    offsetof(struct inode_operations,mknod), S_IFSOCK | S_IRWXUGO, 0);

    if (i == 0)
	i = open_namei(upd->sockaddr_un.sun_path, 0, S_IFSOCK, &upd->inode, NULL);

    current->t_regs.ds = old_ds;
    if (i < 0) {
	debug("UNIX: bind: can't open socket %s\n", upd->sockaddr_un.sun_path);
	if (i == -EEXIST)
	    i = -EADDRINUSE;
	return i;
    }
    upd->sockaddr_len = sockaddr_len;	/* now it's legal */

    return 0;
}

static int unix_connect(struct socket *sock,
			struct sockaddr *uservaddr,
			int sockaddr_len, int flags)
{
    struct sockaddr_un sockun;
    struct unix_proto_data *serv_upd;
    struct inode *inode;
    unsigned short old_ds;
    int i;

    if (sockaddr_len <= 0 || sockaddr_len > sizeof(struct sockaddr_un))
	return -EINVAL;

    if (sock->state == SS_CONNECTING)
	return -EINPROGRESS;

/*    if (sock->state == SS_CONNECTED)
	return -EISCONN;*/	/*Already checked in socket.c*/

    if (get_user(&(((struct sockaddr_un *)uservaddr)->sun_family)) != AF_UNIX)
	return -EINVAL;

    memcpy_fromfs(&sockun, uservaddr, sockaddr_len);
    sockun.sun_path[sockaddr_len] = '\0';

/*
 * Try to open the name in the filesystem - this is how we identify ourselves
 * and our server. Note that we don't hold onto the inode much, just enough to
 * find our server. When we're connected, we mooch off the server.
 */

    old_ds = current->t_regs.ds;
    current->t_regs.ds = kernel_ds;

    i = open_namei(sockun.sun_path, 2, S_IFSOCK, &inode, NULL);
    current->t_regs.ds = old_ds;

    if (i < 0)
	return i;

    serv_upd = unix_data_lookup(&sockun, sockaddr_len, inode);
    iput(inode);

    if (!serv_upd)
	return -EINVAL;

    if ((i = sock_awaitconn(sock, serv_upd->socket, flags)) < 0)
	return i;

    if (sock->conn) {
	unix_data_ref(UN_DATA(sock->conn));
	UN_DATA(sock)->peerupd = UN_DATA(sock->conn);	/* ref server */
    }

    return 0;
}

static int unix_socketpair(struct socket *sock1, struct socket *sock2)
{
    struct unix_proto_data *upd1 = UN_DATA(sock1), *upd2 = UN_DATA(sock2);

    unix_data_ref(upd1);
    unix_data_ref(upd2);

    upd1->peerupd = upd2;
    upd2->peerupd = upd1;

    return 0;
}

static int unix_accept(struct socket *sock, struct socket *newsock, int flags)
{
    struct socket *clientsock;

/*
 * If there aren't any sockets awaiting connection,
 * then wait for one, unless nonblocking.
 */

    while (!(clientsock = sock->iconn)) {
	if (flags & O_NONBLOCK)
	    return -EAGAIN;

	sock->flags |= SF_WAITDATA;
	interruptible_sleep_on(sock->wait);
	sock->flags &= ~SF_WAITDATA;

	if (current->signal /* & ~current->blocked */ )
	    return -ERESTARTSYS;
    }

/*
 * Great. Finish the connection relative to server and client,
 * wake up the client and return the new fd to the server.
 */

    sock->iconn = clientsock->next;
    clientsock->next = NULL;
    newsock->conn = clientsock;
    clientsock->conn = newsock;
    clientsock->state = SS_CONNECTED;
    newsock->state = SS_CONNECTED;
    unix_data_ref(UN_DATA(clientsock));
    UN_DATA(newsock)->peerupd = UN_DATA(clientsock);
    UN_DATA(newsock)->sockaddr_un = UN_DATA(sock)->sockaddr_un;
    UN_DATA(newsock)->sockaddr_len = UN_DATA(sock)->sockaddr_len;
    wake_up_interruptible(clientsock->wait);

#if UNUSED
    sock_wake_async(clientsock, 0);	/* Don't need */
#endif

    return 0;
}

static int unix_getname(struct socket *sock,
			struct sockaddr *usockaddr,
			int *usockaddr_len, int peer)
{
    struct unix_proto_data *upd;

    if (peer) {
	if (sock->state != SS_CONNECTED)
	    return -EINVAL;
	upd = UN_DATA(sock->conn);
    } else
	upd = UN_DATA(sock);

    return move_addr_to_user((char *)&upd->sockaddr_un, upd->sockaddr_len,
				    (char *)usockaddr, usockaddr_len);
}

static int unix_read(struct socket *sock, char *ubuf, int size, int nonblock)
{
    struct unix_proto_data *upd;
    int todo, avail;

    if ((todo = size) <= 0)
	return 0;

    upd = UN_DATA(sock);

    while (!(avail = UN_BUF_AVAIL(upd))) {
	if (sock->state != SS_CONNECTED)
	    return ((sock->state == SS_DISCONNECTING) ? 0 : -EINVAL);

	if (nonblock)
	    return -EAGAIN;

	sock->flags |= SF_WAITDATA;
	interruptible_sleep_on(sock->wait);
	sock->flags &= ~SF_WAITDATA;

	if (current->signal /* & ~current->blocked */ )
	    return -ERESTARTSYS;
    }

/*
 *	Copy from the read buffer into the user's buffer,
 *	watching for wraparound. Then we wake up the writer.
 */

    down(&upd->sem);
    do {
	int part, cando;

	if (avail <= 0) {
	    printk("UNIX: read: avail is negative (%d)\n", avail);
	    send_sig(SIGKILL, current, 1);
	    return -EPIPE;
	}

	if ((cando = todo) > avail)
	    cando = avail;

	if (cando > (part = UN_BUF_SIZE - upd->bp_tail))
	    cando = part;

	memcpy_tofs(ubuf, upd->buf + upd->bp_tail, cando);
	upd->bp_tail = (upd->bp_tail + cando) & (UN_BUF_SIZE - 1);
	ubuf += cando;
	todo -= cando;

	if (sock->state == SS_CONNECTED) {
	    wake_up_interruptible(sock->conn->wait);
#if UNUSED
	    sock_wake_async(sock->conn, 2);
#endif
	}
	avail = UN_BUF_AVAIL(upd);
    } while (todo && avail);

    up(&upd->sem);

    return (size - todo);
}

static int unix_write(struct socket *sock, char *ubuf, int size, int nonblock)
{
    struct unix_proto_data *pupd;
    int todo, space;

    if ((todo = size) <= 0)
	return 0;

    if (sock->state != SS_CONNECTED) {
	if (sock->state == SS_DISCONNECTING) {
	    send_sig(SIGPIPE, current, 1);
	    return -EPIPE;
	}
	return -EINVAL;
    }

    pupd = UN_DATA(sock)->peerupd;	/* safer than sock->conn */

    while (!(space = UN_BUF_SPACE(pupd))) {
	sock->flags |= SF_NOSPACE;

	if (nonblock)
	    return -EAGAIN;

	sock->flags &= ~SF_NOSPACE;
	interruptible_sleep_on(sock->wait);

	if (current->signal /* & ~current->blocked */ )
	    return -ERESTARTSYS;

	if (sock->state == SS_DISCONNECTING) {
	    send_sig(SIGPIPE, current, 1);
	    return -EPIPE;
	}
    }

/*
 *	Copy from the user's buffer to the write buffer,
 *	watching for wraparound. Then we wake up the reader.
 */

    down(&pupd->sem);
    do {
	int part, cando;

	if (space <= 0) {
	    printk("UNIX: write: space is negative (%d)\n", space);
	    send_sig(SIGKILL, current, 1);
	    return -EPIPE;
	}

	/*
	 *      We may become disconnected inside this loop, so watch
	 *      for it (peerupd is safe until we close).
	 */

	if (sock->state == SS_DISCONNECTING) {
	    send_sig(SIGPIPE, current, 1);
	    up(&pupd->sem);
	    return -EPIPE;
	}

	if ((cando = todo) > space)
	    cando = space;

	if (cando > (part = UN_BUF_SIZE - pupd->bp_head))
	    cando = part;

	memcpy_fromfs(pupd->buf + pupd->bp_head, ubuf, cando);
	pupd->bp_head = (pupd->bp_head + cando) & (UN_BUF_SIZE - 1);

	ubuf += cando;
	todo -= cando;

	if (sock->state == SS_CONNECTED) {
	    wake_up_interruptible(sock->conn->wait);
#if UNUSED
	    sock_wake_async(sock->conn, 1);
#endif
	}
	space = UN_BUF_SPACE(pupd);
    } while (todo && space);

    up(&pupd->sem);

    return (size - todo);
}

static int unix_select(struct socket *sock, int sel_type)
{
    struct unix_proto_data *upd, *peerupd;

    /*
     *      Handle server sockets specially.
     */

    if (sock->flags & SF_ACCEPTCON) {
	if (sel_type == SEL_IN) {

	    if (sock->iconn)
		return 1;

	    select_wait(sock->wait);

	    return (sock->iconn ? 1 : 0);
	}
	select_wait(sock->wait);

	return 0;
    }

    if (sel_type == SEL_IN) {
	upd = UN_DATA(sock);

	if (UN_BUF_AVAIL(upd))	/* even if disconnected */
	    return 1;

	else if (sock->state != SS_CONNECTED)
	    return 1;

	select_wait(sock->wait);

	return 0;
    }

    if (sel_type == SEL_OUT) {

	if (sock->state != SS_CONNECTED)
	    return 1;

	peerupd = UN_DATA(sock->conn);

	if (UN_BUF_SPACE(peerupd) > 0)
	    return 1;

	select_wait(sock->wait);

	return 0;
    }

    /*
     * Exceptions - SEL_EX
     */

    return 0;
}

static int unix_listen(struct socket *sock, int backlog)
{
    return 0;
}

static int unix_send(struct socket *sock,
		     void *buff, int len, int nonblock, unsigned int flags)
{
    if (flags != 0)
	return -EINVAL;

    return unix_write(sock, (char *) buff, len, nonblock);
}

/*
 *      Receive data. This version of AF_UNIX also lacks MSG_PEEK 8(
 */

static int unix_recv(struct socket *sock,
		     void *buff, int len, int nonblock, unsigned flags)
{
    if (flags != 0)
	return -EINVAL;

    return unix_read(sock, (char *) buff, len, nonblock);
}

struct proto_ops unix_proto_ops = {
    PF_UNIX,
    unix_create,
    unix_dup,
    unix_release,
    unix_bind,
    unix_connect,
    unix_socketpair,
    unix_accept,
    unix_getname,
    unix_read,
    unix_write,
    unix_select,
    NULL,
    unix_listen,
    unix_send,
    unix_recv,
    NULL,			/* sendto */
    NULL,			/* recvfrom */
    NULL,			/* shutdown */
    NULL,			/* setsockopt */
    NULL,			/* getsockopt */
    NULL			/* fcntl */
};

void unix_proto_init(struct net_proto *pro)
{
    sock_register(AF_UNIX, &unix_proto_ops);
}

#endif
