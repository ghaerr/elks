/*
 * NET		Implementation of Socket uinterface
 *
 * File:	socket.c
 *
 * Author:	Alistair Riddoch, <ajr@ecs.soton.ac.uk>
 *
 * 		Based on code from the Linux kernel by
 *		Orest Zborowski, <obz@Kodak.COM>
 *		Ross Biro, <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen, <waltje@uWalt.NL.Mugnet.ORG>
 */

#include <linuxmt/config.h>

#include <linuxmt/errno.h>
#include <linuxmt/socket.h>
#include <linuxmt/net.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>

/*#define CONFIG_SOCK_CLIENTONLY 1
#define CONFIG_SOCK_STREAMONLY 1 */

#define find_protocol_family(_a) 0

#define MAX_SOCK_ADDR 16 /* Must be much bigger for AF_UNIX */

#define get_user(ptr) peekw(current->t_regs.ds,ptr)
#define put_user(val,ptr) pokew(current->t_regs.ds,ptr,val)

static struct proto_ops *pops[NPROTO]= { NULL };

extern struct net_proto protocols[];    /* Network protocols */

struct socket *socki_lookup(inode)
struct inode *inode;
{
	return &inode->u.socket_i;
}
/* FIXME - Could above be replaced by
 * 	#define socki_lookup(_a) (&_a->u.sock_i)
 */

int move_addr_to_kernel(uaddr, ulen, kaddr)
char * uaddr;
int ulen;
char * kaddr;
{
	int err;
	if(ulen<0||ulen>MAX_SOCK_ADDR)
		return -EINVAL;
	if(ulen==0)
		return 0;
	return verified_memcpy_fromfs(kaddr,uaddr,ulen);
}

int move_addr_to_user(kaddr, klen, uaddr, ulen)
char * kaddr;
int klen;
char * uaddr;
int * ulen;
{
	int err;
	int len;

		
	if (err=verified_memcpy_fromfs(&len, ulen, sizeof(*ulen)))
		return err;
	if(len>klen)
		len=klen;
	if(len<0 || len> MAX_SOCK_ADDR)
		return -EINVAL;
	if(len)
	{
		if (err = verified_memcpy_tofs(uaddr,kaddr,len))
			return err;
	}
/*	put_user(len,ulen); */ /* This is pointless isn't it */
	return 0;
}





struct socket *sockfd_lookup(fd, pfile)
int fd;
struct file **pfile;
{
	register struct file *file;
	register struct inode *inode;

	if (fd < 0 || fd >= NR_OPEN || !(file = current->files.fd[fd])) {
		return NULL;
	}

	inode = file->f_inode;
	if (!inode || !inode->i_sock) {
		return NULL;
	}

	if (pfile)
		*pfile = file;

	return socki_lookup(inode);
}

static int sock_read(inode, file, ubuf, size)
struct inode *	inode;
struct file *	file;
register char *	ubuf;
int		size;
{
	register struct socket *sock;
	int err;
	struct iovec iov;
	struct msghdr msg;

	sock = socki_lookup(inode);
#ifndef CONFIG_SOCK_CLIENTONLY
	if (sock->flags & SO_ACCEPTCON) {
		return -EINVAL;
	}
#endif

	if (size<0) {
		return -EINVAL;
	}
	if (size==0) {
		return 0;
	}
	if ((err=verify_area(VERIFY_WRITE,ubuf,size))<0) {
		return err;
	}
	msg.msg_name = NULL;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	iov.iov_base = ubuf;
	iov.iov_len = size;

	return (sock->ops->recvmsg( sock, &msg, size, 0, 0, &msg.msg_namelen));
	/* Fourth ARG in above should be (file->f_flags & O_NONBLOCK) */
}
	

static int sock_write(inode, file, ubuf, size)
struct inode *	inode;
struct file *	file;
register char *		ubuf;
int		size;
{
	register struct socket *sock;
	int err;
	struct msghdr msg;
	struct iovec iov;

	sock = socki_lookup(inode);
#ifndef CONFIG_SOCK_CLIENTONLY
	if (sock->flags & SO_ACCEPTCON) {
		return -EINVAL;
	}
#endif

	if (size<0) {
		return -EINVAL;
	}
	if (size==0) {
		return 0;
	}
	
	if ((err=verify_area(VERIFY_READ,ubuf,size))<0) {
		return err;
	}
	msg.msg_name = NULL;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = NULL;
	iov.iov_base = ubuf;
	iov.iov_len = size;

	return(sock->ops->sendmsg(sock, &msg, size, 0, 0));
	/* Fourth ARG in above should be (file->f_flags & O_NONBLOCK) */
}

#ifdef CONFIG_UNIX
static void sock_release_peer(peer)
struct socket * peer;
{
	/* FIXME - some of these are not implemented */
	peer->state = SS_DISCONNECTING;
	/* wake_up_interruptible(peer->wait);
	 * sock_wake_async(peer, 1);
	 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^ --- both not yet implemented
	 */
}
#endif /* CONFIG_UNIX */

void sock_release(sock)
register struct socket * sock;
{
	int oldstate;
	register struct socket * peersock;
#ifdef CONFIG_UNIX
	struct socket * nextsock;
#endif

	if ((oldstate = sock->state) != SS_UNCONNECTED) {
		sock->state = SS_DISCONNECTING;
	}

#ifdef CONFIG_UNIX
	for (peersock = sock->iconn; peersock; peersock = nextsock) {
		nextsock = peersock->next;
		sock_release_peer(peersock);
	}

	peersock = (oldstate == SS_CONNECTED) ? sock->conn : NULL;
#else /* CONFIG_UNIX */
	peersock = NULL; /* sock-conn is always NULL for an INET socket */
#endif /* CONFIG_UNIX */
	if (sock->ops)
		sock->ops->release(sock, peersock);
#ifdef CONFIG_UNIX
	if (peersock)
		sock_release_peer(peersock);
#endif
/*	--sockets_in_use; */ /* maybe we'll find a use for this */
	sock->file = NULL;
	iput(SOCK_INODE(sock));
}

void sock_close(inode, filp)
register struct inode * inode;
struct file * filp;
{
	if (!inode) {
		return;
	}
/*	sock_fasync(inode, filp, 0); */
	sock_release(socki_lookup(inode));
}

#ifndef CONFIG_SOCK_CLIENTONLY /* FIXME - all of these */

int sys_bind(fd, umyaddr, addrlen)
int fd;
struct sockaddr *umyaddr;
int addrlen;
{
}

int sys_listen(fd, backlog)
int fd;
int backlog;
{
}

int sys_accept(fd, upeer_sockaddr, upeer_addrlen)
int fd;
struct sockaddr *upeer_sockaddr;
int *upeer_addrlen;
{
}

#endif

int sys_connect(fd, uservaddr, addrlen)
int fd;
struct sockaddr * uservaddr;
int addrlen;
{
	register struct socket * sock;
	struct file * file;
	int i;
	char address[MAX_SOCK_ADDR];
	int err;

/* 	All this is done in sockfd_lookup */
/*	if (fd < 0 || fd >= NR_OPEN || (file=current->files.fd[fd]) == NULL) {
		return -EBADF;
	} */

	if (!(sock = sockfd_lookup(fd, &file))) {
		return -ENOTSOCK;
	}
	if ((err = move_addr_to_kernel(uservaddr,addrlen,address))<0) {
		return err;
	}

	switch(sock->state)
	{
		case SS_UNCONNECTED:
			/* This is ok... continue with connect */
			break;
		case SS_CONNECTED:
			/* Socket is already connected */
#ifndef CONFIG_SOCK_STREAMONLY
			if(sock->type == SOCK_DGRAM) /* Hack for now - move this
 all into the protocol */
				break;
#endif
			return -EISCONN;
		case SS_CONNECTING:
			/* Not yet connected... we will check this. */
		
			/*
			 *      FIXME:  for all protocols what happens if you st
art
			 *      an async connect fork and both children connect.
 Clean
			 *      this up in the protocols!
			 */
			break;
		default:
			return -EINVAL;
	}
	i = sock->ops->connect(sock, (struct sockaddr *)address, addrlen, file->f_flags);
	if (i < 0) {
		return i;
	}
	return 0;
}

int sock_fcntl(filp, cmd, arg)
struct file * filp;
unsigned int cmd;
unsigned long arg;
{
	/* FIXME */
	return -EINVAL;
}

int sock_register(family, ops)
int family;
struct proto_ops *ops;
{
	int i;

	for(i = 0; i < NPROTO; i++) {
		if (pops[i] != NULL)
			continue;
		pops[i] = ops;
		pops[i]->family = family;
		return(i);
	}
	return -ENOMEM;
}

void proto_init()
{
	struct net_proto *pro;

	/* Kick all configured protocols. */
	pro = protocols;
	while (pro->name != NULL) 
	{
		(*pro->init_func)(pro);
		pro++;
	}
	/* We're all done... */
}

void sock_init()
{
	int i;

	printk("ELKS Sockets.\n");

	proto_init();
}

static struct file_operations socket_file_ops = {
	NULL,		/* lseek */
	sock_read,		/* read */
	sock_write,		/* write */
	NULL,			/* readdir */
#ifdef BLOAT_FS
	sock_select,		/* select */
#endif
	NULL,			/* ioctl */
	NULL,			/* open */
	sock_close		/* close */
#ifdef BLOAT_FS
	,NULL,			/* fsync */
	NULL,			/* check_media_change */
	NULL			/* revalidate */
#endif
};

int get_fd(inode)
register struct inode * inode;
{
	int fd;

	fd = get_unused_fd();
	if (fd>=0) {
		struct file *file = get_empty_filp();
		if (!file) {
			return -ENFILE;
		}

		current->files.fd[fd] = file;
		file->f_op = &socket_file_ops;
		file->f_mode = 3;
		file->f_flags = O_RDWR;
		file->f_count = 1;
		file->f_inode = inode;
		if (inode)
			inode->i_count++;
		file->f_pos = 0;
	}
	return fd;
}

struct socket *sock_alloc()
{
	register struct inode * inode;
	register struct socket * sock;

	inode = get_empty_inode();
	if (!inode)
		return NULL;

	inode->i_mode = S_IFSOCK;
	inode->i_sock = 1;
	inode->i_uid = current->uid;
	inode->i_gid = current->gid;

	sock = &inode->u.socket_i;
	sock->state = SS_UNCONNECTED;
	sock->flags = 0;
	sock->ops = NULL;
	sock->data = NULL;
#ifdef CONFIG_UNIX /* Not used by INET sockets */
	sock->conn = NULL;
	sock->iconn = NULL;
	sock->next = NULL;
#endif /* CONFIG_UNIX */
	sock->file = NULL;
	sock->wait = &inode->i_wait;
	sock->inode = inode;            /* "backlink": we could use pointer arithmetic instead */
	sock->fasync_list = NULL;
/*	sockets_in_use++; */ /* Maybe we'll find a use for this */
	return sock;
}

int sys_socket(family, type, protocol)
int family;
int type;
int protocol;
{
	int i, fd;
	register struct socket *sock;
	register struct proto_ops *ops;

/*	find_protocol_family() is a macro which gives 0 while only
 *	AF_INET sockets are supported */

	if ((i = find_protocol_family(family)) < 0) {
		return -EINVAL;
	}
	ops = pops[i];	/* Initially pops is not an array. */

/*	Only TCP supported initially */

	if (type != SOCK_STREAM) {
		return -EINVAL;
	}

	if (!(sock = sock_alloc())) {
		return(-ENOSR);
	}

	sock->type = type;
	sock->ops = ops;
	if ((i = sock->ops->create(sock, protocol)) < 0) {
		sock_release(sock);
		return(i);
	}
	
	if ((fd = get_fd(SOCK_INODE(sock))) < 0) {
		sock_release(sock);
		return(-EINVAL);
	}

	sock->file = current->files.fd[fd];

	return(fd);
}

