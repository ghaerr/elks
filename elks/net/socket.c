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

#ifdef CONFIG_SOCKET

#include <linuxmt/errno.h>
#include <linuxmt/socket.h>
#include <linuxmt/net.h>
#include <linuxmt/fs.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>

#include <arch/segment.h>

/*#define CONFIG_SOCK_CLIENTONLY 1
#define CONFIG_SOCK_STREAMONLY 1 */

/*#define find_protocol_family(_a) 0*/

#define MAX_SOCK_ADDR 16 /* Must be much bigger for AF_UNIX */

static struct proto_ops *pops[NPROTO]= { NULL };

extern struct net_proto protocols[];    /* Network protocols */

static int find_protocol_family(family)
int family;
{
	int i;
	
	for(i = 0 ; i < NPROTO ; i++ )
		if(pops[i]->family == family)return i;
	
	return -1;
}

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
register int * ulen;
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
#if defined(CONFIG_INET)
	sock->avail_data = 0;
#endif	
#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
	sock->conn = NULL;
	sock->iconn = NULL;
	sock->next = NULL;
#endif /* CONFIG_UNIX || CONFIG_NANO */
	sock->file = NULL;
	sock->wait = &inode->i_wait;
	sock->inode = inode;            /* "backlink": we could use pointer arithmetic instead */
	sock->fasync_list = NULL;
/*	sockets_in_use++; */ /* Maybe we'll find a use for this */
	return sock;
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
	struct socket *sock;
	int err;
  
	if (!(sock = socki_lookup(inode))) 
	{
		printk("NET: sock_read: can't find socket for inode!\n");
		return(-EBADF);
	}
	if (sock->flags & SO_ACCEPTCON) 
		return(-EINVAL);

	if(size<0)
		return -EINVAL;
	if(size==0)
		return 0;
	if ((err=verify_area(VERIFY_WRITE,ubuf,size))<0)
	  	return err;
	return(sock->ops->read(sock, ubuf, size, (file->f_flags & O_NONBLOCK)));
}
	

static int sock_write(inode, file, ubuf, size)
struct inode *	inode;
struct file *	file;
register char *		ubuf;
int		size;
{
	struct socket *sock;
	int err;
	
	if (!(sock = socki_lookup(inode))) 
	{
		printk("NET: sock_write: can't find socket for inode!\n");
		return(-EBADF);
	}

	if (sock->flags & SO_ACCEPTCON) 
		return(-EINVAL);
	
	if(size<0)
		return -EINVAL;
	if(size==0)
		return 0;
		
	if ((err=verify_area(VERIFY_READ,ubuf,size))<0)
	  	return err;
	return(sock->ops->write(sock, ubuf, size,(file->f_flags & O_NONBLOCK)));
}

static int sock_select(inode, file, sel_type, wait)
struct inode * inode;
struct file * file;
int sel_type;
select_table * wait;
{
	register struct socket *sock;
	
	if (!(sock = socki_lookup(inode))) {
		return -EBADF;
	}

	if (sock->ops && sock->ops->select) {
		return (sock->ops->select(sock, sel_type, wait));
	}
	return 0;
}

int sock_awaitconn(mysock, servsock, flags)
register struct socket *mysock;
struct socket *servsock;
int flags;
{
	register struct socket *last;

	/*
	 *	We must be listening
	 */
	if (!(servsock->flags & SO_ACCEPTCON)) 
	{
		return(-EINVAL);
	}

  	/*
  	 *	Put ourselves on the server's incomplete connection queue. 
  	 */
  	 
	mysock->next = NULL;
	icli();
	if (!(last = servsock->iconn)) 
		servsock->iconn = mysock;
	else 
	{
		while (last->next) 
			last = last->next;
		last->next = mysock;
	}
	mysock->state = SS_CONNECTING;
	mysock->conn = servsock;
	isti();

	/*
	 * Wake up server, then await connection. server will set state to
	 * SS_CONNECTED if we're connected.
	 */
	wake_up_interruptible(servsock->wait);
/*	sock_wake_async(servsock, 0); */ /* I don't think we need this */

	if (mysock->state != SS_CONNECTED) 
	{
		if (flags & O_NONBLOCK)
			return -EINPROGRESS;

		interruptible_sleep_on(mysock->wait);
		if (mysock->state != SS_CONNECTED &&
		    mysock->state != SS_DISCONNECTING) 
		{
		/*
		 * if we're not connected we could have been
		 * 1) interrupted, so we need to remove ourselves
		 *    from the server list
		 * 2) rejected (mysock->conn == NULL), and have
		 *    already been removed from the list
		 */
			if (mysock->conn == servsock) 
			{
				icli();
				if ((last = servsock->iconn) == mysock)
					servsock->iconn = mysock->next;
				else 
				{
					while (last->next != mysock) 
						last = last->next;
					last->next = mysock->next;
				}
				isti();
			}
			return(mysock->conn ? -EINTR : -EACCES);
		}
	}
	return(0);
}


#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
static void sock_release_peer(peer)
struct socket * peer;
{
	/* FIXME - some of these are not implemented */
	peer->state = SS_DISCONNECTING;
	wake_up_interruptible(peer->wait);
/*	sock_wake_async(peer, 1); */
}
#endif /* CONFIG_UNIX || CONFIG_NANO */

void sock_release(sock)
register struct socket * sock;
{
	int oldstate;
	register struct socket * peersock;
#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
	struct socket * nextsock;
#endif

	if ((oldstate = sock->state) != SS_UNCONNECTED) {
		sock->state = SS_DISCONNECTING;
	}

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
	for (peersock = sock->iconn; peersock; peersock = nextsock) {
		nextsock = peersock->next;
		sock_release_peer(peersock);
	}

	peersock = (oldstate == SS_CONNECTED) ? sock->conn : NULL;
#else /* CONFIG_UNIX || CONFIG_NANO */
	peersock = NULL; /* sock-conn is always NULL for an INET socket */
#endif /* CONFIG_UNIX || CONFIG_NANO */
	if (sock->ops)
		sock->ops->release(sock, peersock);
#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
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
	register struct socket *sock;
	int i;
	char address[MAX_SOCK_ADDR];
	int err;

	/* This is done in sockfd_lookup, so can be scrubbed later */
/*	if (fd < 0 || fd >= NR_OPEN || (current->files.fd[fd] == NULL)) {
		return(-EBADF);
	} */
	if (!(sock = sockfd_lookup(fd, NULL))) {
		return(-ENOTSOCK);
	}
	if ((err = move_addr_to_kernel(umyaddr,addrlen,address))<0) {
		return err;
	}
	if ((i=sock->ops->bind(sock,(struct sockaddr *)address,addrlen)) < 0) {
		return(i);
	}

	return(0);

}

int sys_listen(fd, backlog)
int fd;
int backlog;
{
	register struct socket *sock;

	/* This is done in sockfd_lookup, so can be scrubbed later */
/*	if (fd < 0 || fd >= NR_OPEN || current->files.fd[fd] == NULL) {
		return(-EBADF);
	} */

	if (!(sock = sockfd_lookup(fd, NULL))) {
		return(-ENOTSOCK);
	}

	if (sock->state != SS_UNCONNECTED) {
		return(-EINVAL);
	}

	if (sock->ops && sock->ops->listen) {
		sock->ops->listen(sock, backlog);
	}
	sock->flags |= SO_ACCEPTCON;
	return(0);

}

int sys_accept(fd, upeer_sockaddr, upeer_addrlen)
int fd;
struct sockaddr *upeer_sockaddr;
int *upeer_addrlen;
{
	struct file *file;
	register struct socket *sock;
	register struct socket *newsock;
	int i;
	char address[MAX_SOCK_ADDR];
	int len;

	/* This is done in sockfd_lookup, so can be scrubbed later */
/*	if (fd < 0 || fd >= NR_OPEN || current->files.fd[fd] == NULL) {
		return(-EBADF);
	} */
  	if (!(sock = sockfd_lookup(fd, &file))) {
		return(-ENOTSOCK);
	}
	if (sock->state != SS_UNCONNECTED) {
		return(-EINVAL);
	}
	if (!(sock->flags & SO_ACCEPTCON)) {
		return(-EINVAL);
	}

	if (!(newsock = sock_alloc())) {
		printk("NET: sock_accept: no more sockets\n");
		return(-ENOSR);	/* Was: EAGAIN, but we are out of system
				   resources! */
	}
	newsock->type = sock->type;
	newsock->ops = sock->ops;
	if ((i = sock->ops->dup(newsock, sock)) < 0) {
		sock_release(newsock);
		return(i);
	}

	i = newsock->ops->accept(sock, newsock, file->f_flags);
	if ( i < 0) {
		sock_release(newsock);
		return(i);
	}

	if ((fd = get_fd(SOCK_INODE(newsock))) < 0) {
		sock_release(newsock);
		return(-EINVAL);
	}

	if (upeer_sockaddr) {
		newsock->ops->getname(newsock, (struct sockaddr *)address, &len, 1);
		move_addr_to_user(address,len, upeer_sockaddr, upeer_addrlen);
	}
	return(fd);

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
	sock_select,		/* select */
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
		printk("fail family : %d\n",i);
		panic("fuc");
		return -EINVAL;
	}

	ops = pops[i];	/* Initially pops is not an array. */

#ifdef CONFIG_SOCK_STREAMONLY
	if (type != SOCK_STREAM) {
		return -EINVAL;
	}
#endif

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


#endif
