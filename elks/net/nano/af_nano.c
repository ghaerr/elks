/*
 * net/nano/af_nano.c
 *
 * (C) 1999 Al Riddoch <ajr@ecs.soton.ac.uk>
 *
 * A bare implentation of the features of NANO sockets required by nano-X,
 * no more, no less.
 */

#include <linuxmt/errno.h>
#include <linuxmt/config.h>
#include <linuxmt/socket.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/stat.h>
#include <linuxmt/fcntl.h>
#include "af_nano.h"

#ifdef CONFIG_NANO

struct nano_proto_data nano_datas[NSOCKETS_NANO];

static struct nano_proto_data * nano_data_alloc()
{
	register struct nano_proto_data *upd;

	icli();
	for(upd = nano_datas; upd <= last_nano_data; ++upd) {
		if (!upd->npd_refcnt) {
			upd->npd_refcnt = -1;       /* nano domain socket not yet in
			itialised - bgm */
			isti();
			upd->npd_socket = NULL;
			upd->npd_sockaddr_len = 0;
			upd->npd_sockaddr_na.sun_family = 0;
/*			upd->npd_buf = NULL; */
			upd->npd_bp_head = upd->npd_bp_tail = 0;
			upd->npd_srvno = 0;
			upd->npd_peerupd = NULL;
			return(upd);
		}
	}
	isti();
	return(NULL);
}

static struct nano_proto_data * nano_data_lookup(sockna, serv_no)
struct sockaddr_na *sockna;
int serv_no;
{
	register struct nano_proto_data *upd;

	printk("ndl [%d] ", serv_no);
	for(upd = nano_datas; upd <= last_nano_data; ++upd) {
		printk("%d %d %d %d\n", upd->npd_refcnt, upd->npd_socket->state, SS_UNCONNECTED, upd->npd_srvno);
		if (upd->npd_refcnt > 0 &&  upd->npd_socket &&
			upd->npd_socket->state == SS_UNCONNECTED &&
			upd->npd_sockaddr_na.sun_family == sockna->sun_family && 
			upd->npd_srvno == serv_no) {
			return(upd);
		}
	}
	return(NULL);
}


static void nano_data_ref(upd)
register struct nano_proto_data *upd;
{
	if (!upd) 
	{
		return;
	}
	++upd->npd_refcnt;
}

static void nano_data_deref(upd)
register struct nano_proto_data *upd;
{
	if (upd->npd_refcnt == 1) {
		upd->npd_bp_head = upd->npd_bp_tail = 0;
	}
	--upd->npd_refcnt;
}

static void nano_lock(upd)
register struct nano_proto_data *upd;
{
	while (upd->npd_lock_flag) {
		sleep_on(&upd->npd_wait);
	}
	upd->npd_lock_flag = 1;
}


static void nano_unlock(upd)
register struct nano_proto_data *upd;
{
	upd->npd_lock_flag = 0;
	wake_up(&upd->npd_wait);
}


static int nano_create(sock, protocol)
register struct socket * sock;
int protocol;
{
	register struct nano_proto_data * upd;

	printk("nano_create %x\n", sock);
	if (protocol != 0) {
		return -EINVAL;
	}

	if (!(upd = nano_data_alloc())) {
		return -ENOMEM;
	}
/*	upd->protocol = protocol; */ /* I don't think this is necessary */
	upd->npd_socket = sock;
	sock->data = upd;
	upd->npd_refcnt = 1;
	return 0;
}

static int nano_dup(newsock, oldsock)
struct socket *newsock;
struct socket *oldsock;
{
/*	struct nano_proto_data *upd = oldsock->data; */
	return(nano_create(newsock, 0/* upd->protocol */));
}


static int nano_release(sock, peer)
register struct socket *sock;
struct socket *peer;
{
        register struct nano_proto_data *upd = sock->data;

	printk("nano_release %x\n", sock);
        if (!upd) {
                return(0);
	}

        if (upd->npd_socket != sock) {
                printk("NANO: release: socket link mismatch!\n");
                return(-EINVAL);
        }

        if (upd->npd_srvno) {
                upd->npd_srvno = 0;
        }

        sock->data = NULL;
        upd->npd_socket = NULL;

        if (upd->npd_peerupd)
                nano_data_deref(upd->npd_peerupd);
        nano_data_deref(upd);
        return(0);
}

static int nano_bind(sock, umyaddr, sockaddr_len)
struct socket *sock;
struct sockaddr *umyaddr;
int sockaddr_len;
{
	int ano;
	register struct nano_proto_data *upd = sock->data;
	register struct nano_proto_data *pupd;
	unsigned short old_ds;
	int i;

	printk("nano_bind %x\n", sock);
	if (sockaddr_len <= 0 || sockaddr_len > sizeof(struct sockaddr_na)) {
		return(-EINVAL);
	}
	if (upd->npd_sockaddr_len || upd->npd_srvno) {
		/* already bound */
		return(-EINVAL);
	}

	memcpy(&upd->npd_sockaddr_na, umyaddr, sockaddr_len);

	if (upd->npd_sockaddr_na.sun_family != AF_NANO) 
	{
		return(-EINVAL);
	}

	for (pupd = nano_datas; pupd <= last_nano_data; pupd++) {
		if (pupd->npd_srvno == upd->npd_sockaddr_na.sun_no) {
			return(-EADDRINUSE);
		}
	}
	upd->npd_srvno = upd->npd_sockaddr_na.sun_no;
	upd->npd_sockaddr_len = sockaddr_len;	/* now it's legal */
	
	return(0);
}

static int nano_connect(sock, uservaddr, sockaddr_len, flags)
register struct socket *sock;
struct sockaddr *uservaddr;
int sockaddr_len;
int flags;
{
	struct sockaddr_na sockna;
	struct nano_proto_data *serv_upd;
	int i;

	printk("nano_connect %x\n", sock);
	if (sockaddr_len <= 0 || sockaddr_len > sizeof(struct sockaddr_na)) {
		return(-EINVAL);
	}

	if (sock->state == SS_CONNECTING) 
		return(-EINPROGRESS);
	if (sock->state == SS_CONNECTED)
		return(-EISCONN);

	memcpy(&sockna, uservaddr, sockaddr_len);

	if (sockna.sun_family != AF_NANO) {
		printk("ADR - {%d}\n", sockna.sun_family);
		return(-EINVAL);
	}
	  
	serv_upd = nano_data_lookup(&sockna, sockna.sun_no);
	if (!serv_upd) {
		return(-EINVAL);
	}
	
	if ((i = sock_awaitconn(sock, serv_upd->npd_socket, flags)) < 0) {
		return(i);
	}

	if (sock->conn) {
		nano_data_ref(NA_DATA(sock->conn));
		NA_DATA(sock)->npd_peerupd = NA_DATA(sock->conn); /* ref server */
	}
	return(0);
}

static int nano_socketpair(sock1, sock2)
struct socket *sock1;
struct socket *sock2;
{
	register struct nano_proto_data *upd1 = NA_DATA(sock1), *upd2 = NA_DATA(sock2);

	nano_data_ref(upd1);
	nano_data_ref(upd2);
	upd1->npd_peerupd = upd2;
	upd2->npd_peerupd = upd1;
	return(0);
}

static int nano_accept(sock, newsock, flags)
register struct socket *sock;
struct socket *newsock;
int flags;
{
	register struct socket *clientsock;

/*
 * If there aren't any sockets awaiting connection,
 * then wait for one, unless nonblocking.
 */

	printk("nano_accept %x %x\n", sock, newsock);
	while(!(clientsock = sock->iconn)) 
	{
		if (flags & O_NONBLOCK) 
			return(-EAGAIN);
		sock->flags |= SO_WAITDATA;
		interruptible_sleep_on(sock->wait);
		sock->flags &= ~SO_WAITDATA;
		if (current->signal/* & ~current->blocked*/) {
			return(-ERESTARTSYS);
		}
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
	nano_data_ref(NA_DATA(clientsock));
	NA_DATA(newsock)->npd_peerupd = NA_DATA(clientsock);
	NA_DATA(newsock)->npd_sockaddr_na = NA_DATA(sock)->npd_sockaddr_na;
	NA_DATA(newsock)->npd_sockaddr_len = NA_DATA(sock)->npd_sockaddr_len;
	wake_up_interruptible(clientsock->wait);
/*	sock_wake_async(clientsock, 0); */ /* Don't need */
	return(0);
}

static int nano_getname(sock, usockaddr, usockaddr_len, peer)
register struct socket *sock;
struct sockaddr *usockaddr;
int *usockaddr_len;
int peer;
{
	register struct nano_proto_data *upd;
	int len;

	if (peer) {
		if (sock->state != SS_CONNECTED) {
			return(-EINVAL);
		}
		upd = NA_DATA(sock->conn);
	} else {
		upd = NA_DATA(sock);
	}

	len = upd->npd_sockaddr_len;
	memcpy(usockaddr, &upd->npd_sockaddr_na, len);
 	*usockaddr_len=len;
	return(0);
}

static int nano_read(sock, ubuf, size, nonblock)
register struct socket *sock;
char *ubuf;
int size;
int nonblock;
{
	register struct nano_proto_data *upd;
	int todo, avail;

	/*printk("nano_read %x\n", sock);*/
	if ((todo = size) <= 0) 
		return(0);

	upd = NA_DATA(sock);
	while(!(avail = NA_BUF_AVAIL(upd))) {
		if (sock->state != SS_CONNECTED) {
			return((sock->state == SS_DISCONNECTING) ? 0 : -EINVAL);
		}
		if (nonblock) {
			return(-EAGAIN);
		}
		sock->flags |= SO_WAITDATA;
		interruptible_sleep_on(sock->wait);
		sock->flags &= ~SO_WAITDATA;
		if (current->signal/* & ~current->blocked*/) {
			return(-ERESTARTSYS);
		}
	}
/*
 *	Copy from the read buffer into the user's buffer,
 *	watching for wraparound. Then we wake up the writer.
 */
   
	nano_lock(upd);
	do {
		int part, cando;

		if (avail <= 0) 
		{
			printk("NANO: read: AVAIL IS NEGATIVE!!!\n");
			send_sig(SIGKILL, current, 1);
			return(-EPIPE);
		}

		if ((cando = todo) > avail) 
			cando = avail;
		if (cando >(part = NA_BUF_SIZE - upd->npd_bp_tail)) 
			cando = part;
		memcpy_tofs(ubuf, upd->npd_buf + upd->npd_bp_tail, cando);
		upd->npd_bp_tail =(upd->npd_bp_tail + cando) &(NA_BUF_SIZE-1);
		ubuf += cando;
		todo -= cando;
		if (sock->state == SS_CONNECTED)
		{
			wake_up_interruptible(sock->conn->wait);
/*			sock_wake_async(sock->conn, 2); */
		}
		avail = NA_BUF_AVAIL(upd);
	} 
	while(todo && avail);
	nano_unlock(upd);
	return(size - todo);
}

static int nano_write(sock, ubuf, size, nonblock)
register struct socket *sock;
char *ubuf;
int size;
int nonblock;
{
	register struct nano_proto_data *pupd;
	int todo, space;

	/*printk("nano_write %x\n", sock);*/
	if ((todo = size) <= 0)
		return(0);
	if (sock->state != SS_CONNECTED) 
	{
		if (sock->state == SS_DISCONNECTING) 
		{
			send_sig(SIGPIPE, current, 1);
			return(-EPIPE);
		}
		return(-EINVAL);
	}
	pupd = NA_DATA(sock)->npd_peerupd;	/* safer than sock->conn */

	while(!(space = NA_BUF_SPACE(pupd))) 
	{
		sock->flags |= SO_NOSPACE;
		if (nonblock) 
			return(-EAGAIN);
		sock->flags &= ~SO_NOSPACE;
		interruptible_sleep_on(sock->wait);
		if (current->signal/* & ~current->blocked*/) 
		{
			return(-ERESTARTSYS);
		}
		if (sock->state == SS_DISCONNECTING) 
		{
			send_sig(SIGPIPE, current, 1);
			return(-EPIPE);
		}
	}

/*
 *	Copy from the user's buffer to the write buffer,
 *	watching for wraparound. Then we wake up the reader.
 */
   
	nano_lock(pupd);

	do 
	{
		int part, cando;

		if (space <= 0) 
		{
			printk("NANO: write: SPACE IS NEGATIVE!!!\n");
			send_sig(SIGKILL, current, 1);
			return(-EPIPE);
		}

		/*
		 *	We may become disconnected inside this loop, so watch
		 *	for it (peerupd is safe until we close).
		 */
		 
		if (sock->state == SS_DISCONNECTING) 
		{
			send_sig(SIGPIPE, current, 1);
			nano_unlock(pupd);
			return(-EPIPE);
		}
		
		if ((cando = todo) > space) 
			cando = space;

		if (cando >(part = NA_BUF_SIZE - pupd->npd_bp_head))
			cando = part;
	
		memcpy_fromfs(pupd->npd_buf + pupd->npd_bp_head, ubuf, cando);
		pupd->npd_bp_head =(pupd->npd_bp_head + cando) &(NA_BUF_SIZE-1);
		ubuf += cando;
		todo -= cando;
		if (sock->state == SS_CONNECTED)
		{
			wake_up_interruptible(sock->conn->wait);
/*			sock_wake_async(sock->conn, 1); */
		}
		space = NA_BUF_SPACE(pupd);
	}
	while(todo && space);

	nano_unlock(pupd);
	return(size - todo);
}

static int nano_select(sock, sel_type, wait)
register struct socket *sock;
int sel_type;
select_table * wait;
{
	struct nano_proto_data *upd, *peerupd;

	/* 
	 *	Handle server sockets specially.
	 */
	if (sock->flags & SO_ACCEPTCON) {
		if (sel_type == SEL_IN) 
		{
			if (sock->iconn) 
				return(1);
			select_wait(sock->wait, wait);
			return(sock->iconn ? 1 : 0);
		}
		select_wait(sock->wait, wait);
		return(0);
	}

	if (sel_type == SEL_IN) {
		upd = NA_DATA(sock);
		if (NA_BUF_AVAIL(upd)) {	/* even if disconnected */
			return(1);
		} else if (sock->state != SS_CONNECTED) {
			return(1);
		}
		select_wait(sock->wait,wait);
		return(0);
	}

	if (sel_type == SEL_OUT) 
	{
		if (sock->state != SS_CONNECTED) {
			return(1);
		}
		peerupd = NA_DATA(sock->conn);
		if (NA_BUF_SPACE(peerupd) > 0) 
			return(1);
		select_wait(sock->wait,wait);
		return(0);
	}

	/*
	 * Exceptions - SEL_EX 
	 */

	return(0);
}

/* As far as I am aware this function is totally unnecessary */
static int nano_listen(sock, backlog)
struct socket *sock;
int backlog;
{
	printk("nano_listen %x\n", sock);
	return(0);
}



static int nano_send(sock, buff, len, nonblock, flags)
struct socket *sock;
void *buff;
int len;
int nonblock;
unsigned int flags;
{
        if (flags != 0) 
                return(-EINVAL);
        return(nano_write(sock, (char *) buff, len, nonblock));
}

/* 
 *      Receive data. This version of AF_NANO also lacks MSG_PEEK 8(
 */
 
static int nano_recv(sock, buff, len, nonblock, flags)
struct socket *sock;
void *buff;
int len;
int nonblock;
unsigned flags;
{
	if (flags != 0) {
		return(-EINVAL);
	}
	return(nano_read(sock, (char *) buff, len, nonblock));
}


struct proto_ops nano_proto_ops = {
	PF_NANO,
	
	nano_create,
	nano_dup,
	nano_release,
	nano_bind,
	nano_connect,
	nano_socketpair,
	nano_accept,
	nano_getname,
	nano_read,
	nano_write,
	nano_select,
	NULL,
	nano_listen,
	nano_send,
	nano_recv,
	NULL,			/* sendto */
	NULL,			/* recvfrom */
	NULL,			/* shutdown */
	NULL,			/* setsockopt */
	NULL,			/* getsockopt */
	NULL			/* fcntl */
};

void nano_proto_init(pro)
struct net_proto * pro;
{
	printk("ELKS NANO domain Sockets\n");
	sock_register(AF_NANO,&nano_proto_ops);
};

	
#endif /* CONFIG_NANO */
