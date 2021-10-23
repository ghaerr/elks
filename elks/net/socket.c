/*
 * NET		Implementation of Socket interface
 *
 * File:	socket.c
 *
 * Author:	Alistair Riddoch, <ajr@ecs.soton.ac.uk>
 *
 * 		Based on code from the Linux kernel by
 *		Orest Zborowski <obz@Kodak.COM>
 *		Ross Biro <bir7@leland.Stanford.Edu>
 *		Fred N. van Kempen <waltje@uWalt.NL.Mugnet.ORG>
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
#include <linuxmt/debug.h>

#include <arch/segment.h>

#define socki_lookup(inode)	(&inode->u.socket_i)

static struct proto_ops *pops[NPROTO] = { NULL, NULL, NULL };

extern struct net_proto protocols[];	/* Network protocols */

static struct proto_ops *find_protocol_family(int family)
{
    register struct proto_ops **props;

    for (props = pops; props < &pops[NPROTO]; props++)
	if ((*props != NULL) && ((*props)->family == family))
	    return *props;

    return NULL;
}

static int check_addr_to_kernel(void *uaddr, size_t ulen)
{
    if (ulen > MAX_SOCK_ADDR)
	return -EINVAL;

    if (ulen == 0)
	return 0;

    return verify_area(VERIFY_READ, uaddr, ulen);
}

int move_addr_to_user(char *kaddr, size_t klen, char *uaddr, int *ulen)
{
    size_t len;
    int err = 0;

    if (verify_area(VERIFY_READ, ulen, sizeof(int)))
	return -EFAULT;
    len = get_user(ulen);

    if (len > klen)		/* If len < klen, truncate data */
	len = klen;

/*    if (len > MAX_SOCK_ADDR)
	return -EINVAL;*/	/* Now, this is the pointless code */

    if (len)
	err = verified_memcpy_tofs(uaddr, kaddr, len);
    return err;
}

static struct socket *sock_alloc(void)
{
    static struct socket ini_sock = {
	0,		/* type */
	SS_UNCONNECTED, /* state */
	0,		/* flags */
	NULL,		/* ops */
	NULL,		/* data */
#if defined(CONFIG_INET)
	0,		/* avail_data */
	0,		/* sem */
#endif
#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
	NULL,		/* conn */
	NULL,		/* iconn */
	NULL,		/* next */
#endif
	NULL,		/* wait */
	NULL,		/* inode */
	NULL,		/* fasync_list */
	NULL,		/* file */
    };
    register struct inode *inode;
    register struct socket *sock;

    if (!(inode = new_inode(NULL, S_IFSOCK)))
	return NULL;

    sock = &inode->u.socket_i;

    *sock = ini_sock;

    sock->wait = &inode->i_wait;
    sock->inode = inode;	/* "backlink" use pointer arithmetic instead */

    return sock;
}

static struct socket *sockfd_lookup(int fd, struct file **pfile)
{
    register struct file *file;
    register struct inode *inode;

    if (((unsigned int)fd >= NR_OPEN) || !(file = current->files.fd[fd]))
	return NULL;	/* could differentiate between EBADF but doesn't*/

    inode = file->f_inode;
    if (!inode || ((inode->i_mode & S_IFMT) != S_IFSOCK))
	return NULL;	/* will be ENOTSOCK*/

    if (pfile)
	*pfile = file;

    return &inode->u.socket_i;
}

static size_t sock_read(struct inode *inode, struct file *file,
		     char *ubuf, size_t size)
{
    register struct socket *sock;
    int err;

    if (!(sock = socki_lookup(inode))) {
	debug_net("NET(%d) can't find sock_read socket\n", current->pid);
	return -EBADF;
    }

    if (sock->flags & SO_ACCEPTCON)
	return -EINVAL;

    if (size == 0)
	return 0;

    if ((err = verify_area(VERIFY_WRITE, ubuf, size)) < 0)
	return err;

    return sock->ops->read(sock, ubuf, size, (file->f_flags & O_NONBLOCK));
}

static size_t sock_write(struct inode *inode, struct file *file,
		      char *ubuf, size_t size)
{
    register struct socket *sock;
    int err;

    if (!(sock = socki_lookup(inode))) {
	debug_net("NET(%d) can't find sock_write socket\n", current->pid);
	return -EBADF;
    }

    if (sock->flags & SO_ACCEPTCON)
	return -EINVAL;

    if (size == 0)
	return 0;

    if ((err = verify_area(VERIFY_READ, ubuf, size)) < 0)
	return err;

    return sock->ops->write(sock, ubuf, size, (file->f_flags & O_NONBLOCK));
}

static int sock_select(struct inode *inode, struct file *file, int sel_type)
{
    register struct socket *sock;
    register struct proto_ops *ops;

    if (!(sock = socki_lookup(inode)))
	return -EBADF;

    ops = sock->ops;
    if (ops && ops->select)
	return ops->select(sock, sel_type);

    return 0;
}

int sock_awaitconn(register struct socket *mysock,
		   struct socket *servsock, int flags)
{
    register struct socket *last;

    /*
     *      We must be listening
     */
    if (!(servsock->flags & SO_ACCEPTCON))
	return -EINVAL;

    /*
     *      Put ourselves on the server's incomplete connection queue.
     */
    mysock->next = NULL;

    clr_irq();
    if (!(last = servsock->iconn))
	servsock->iconn = mysock;
    else {
	while (last->next)
	    last = last->next;
	last->next = mysock;
    }
    mysock->state = SS_CONNECTING;
    mysock->conn = servsock;
    set_irq();

    /*
     * Wake up server, then await connection. server will set state to
     * SS_CONNECTED if we're connected.
     */

    wake_up_interruptible(servsock->wait);
    /*sock_wake_async(servsock, 0);*/

    if (mysock->state != SS_CONNECTED) {
	if (flags & O_NONBLOCK)
	    return -EINPROGRESS;

	interruptible_sleep_on(mysock->wait);
	if (mysock->state != SS_CONNECTED && mysock->state != SS_DISCONNECTING) {
	    /*
	     * if we're not connected we could have been
	     * 1) interrupted, so we need to remove ourselves
	     *    from the server list
	     * 2) rejected (mysock->conn == NULL), and have
	     *    already been removed from the list
	     */
	    if (mysock->conn == servsock) {
		clr_irq();
		if ((last = servsock->iconn) == mysock)
		    servsock->iconn = mysock->next;
		else {
		    while (last->next != mysock)
			last = last->next;
		    last->next = mysock->next;
		}
		set_irq();
	    }
	    if (mysock->conn)
		return -EINTR;
	    else
		return -EACCES;
	}
    }
    return 0;
}


#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
static void sock_release_peer(register struct socket *peer)
{
    /* FIXME - some of these are not implemented */
    peer->state = SS_DISCONNECTING;
    wake_up_interruptible(peer->wait);
    /*sock_wake_async(peer, 1);*/
}
#endif

static void sock_release(register struct socket *sock)
{
    int oldstate;
    register struct socket *peersock;

    if ((oldstate = sock->state) != SS_UNCONNECTED)
	sock->state = SS_DISCONNECTING;

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
    struct socket *nextsock;
    for (peersock = sock->iconn; peersock; peersock = nextsock) {
	nextsock = peersock->next;
	sock_release_peer(peersock);
    }

    peersock = (oldstate == SS_CONNECTED) ? sock->conn : NULL;
#else
    peersock = NULL;		/* sock-conn is always NULL for an INET socket */
#endif

    if (sock->ops)
	sock->ops->release(sock, peersock);

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO) || defined(CONFIG_INET)
    if (peersock)
	sock_release_peer(peersock);
#endif

    sock->file = NULL;
    iput(SOCK_INODE(sock));
}

static void sock_close(register struct inode *inode, struct file *filp)
{
    if (!inode)
	return;

    /*sock_fasync(inode, filp, 0);*/
    sock_release(&inode->u.socket_i);
}

int sys_bind(int fd, struct sockaddr *umyaddr, int addrlen)
{
    register struct socket *sock;
    int err;

    if (!(sock = sockfd_lookup(fd, NULL)))
	return -ENOTSOCK;

    err = check_addr_to_kernel(umyaddr, addrlen);
    if (err < 0)
	return err;

    if ((err = sock->ops->bind(sock, umyaddr, addrlen)) < 0)
	return err;

    return 0;
}

/*@-type@*/

static struct file_operations socket_file_ops = {
    NULL,			/* lseek */
    sock_read,			/* read */
    sock_write,			/* write */
    NULL,			/* readdir */
    sock_select,		/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    sock_close			/* close */
#ifdef BLOAT_FS
	,
    NULL,			/* fsync */
    NULL,			/* check_media_change */
    NULL			/* revalidate */
#endif
};

struct inode_operations sock_inode_operations = {
    &socket_file_ops, NULL,	/* create */
    NULL,			/* lookup */
    NULL,			/* link */
    NULL,			/* unlink */
    NULL,			/* symlink */
    NULL,			/* mkdir */
    NULL,			/* rmdir */
    NULL,			/* mknod */
    NULL,			/* readlink */
    NULL,			/* follow_link */
#ifdef USE_GETBLK
    NULL,			/* getblk */
#endif
    NULL,			/* truncate */
#ifdef BLOAT_FS
    NULL			/* permission */
#endif
};

static int get_fd(register struct inode *inode)
{
    int fd;

    if ((fd = open_fd(O_RDWR, inode)) >= 0)
	inode->i_count++;		/*FIXME: Really needed?*/
    return fd;
}

#ifdef CONFIG_SOCK_CLIENTONLY

int sys_listen(int fd, int backlog)
{
    return -EINVAL;
}

int sys_accept(int fd, struct sockaddr *upeer_sockaddr, int *upeer_addrlen)
{
    return -EINVAL;
}

#else

int sys_listen(int fd, int backlog)
{
    register struct socket *sock;
    register struct proto_ops *ops;

    if (!(sock = sockfd_lookup(fd, NULL)))
	return -ENOTSOCK;

    if (sock->state != SS_UNCONNECTED)
	return -EINVAL;

    ops = sock->ops;
    if (ops && ops->listen)
	ops->listen(sock, backlog);

    sock->flags |= SO_ACCEPTCON;
    return 0;

}

int sys_accept(int fd, struct sockaddr *upeer_sockaddr, int *upeer_addrlen)
{
    struct file *file;
    register struct socket *sock;
    register struct socket *newsock;
    int i;

    if (!(sock = sockfd_lookup(fd, &file)))
	return -ENOTSOCK;

    if (sock->state != SS_UNCONNECTED)
	return -EINVAL;

    if (!(sock->flags & SO_ACCEPTCON))
	return -EINVAL;

    if (!(newsock = sock_alloc())) {
	debug_net("NET(%d) sys_accept: no more sockets\n", current->pid);
	return -ENOSR;		/* Was EAGAIN, but we are out of system resources! */
    }

    newsock->type = sock->type;
    newsock->ops = sock->ops;
    if ((i = sock->ops->dup(newsock, sock)) < 0) {
	sock_release(newsock);
	return i;
    }

    i = newsock->ops->accept(sock, newsock, file->f_flags);
    if (i < 0) {
	sock_release(newsock);
	return i;
    }

    if ((fd = get_fd(SOCK_INODE(newsock))) < 0) {
	sock_release(newsock);
	return -EINVAL;
    }

    if (upeer_sockaddr) {
	newsock->ops->getname(newsock, upeer_sockaddr, upeer_addrlen, 1);
    }

    return fd;
}

#endif /* CONFIG_SOCK_CLIENTONLY*/

int sys_connect(int fd, struct sockaddr *uservaddr, int addrlen)
{
    register struct socket *sock;
    struct file *file;
    int err;

    if (!(sock = sockfd_lookup(fd, &file)))
	return -ENOTSOCK;

    err = check_addr_to_kernel(uservaddr, addrlen);
    if (err < 0)
	return err;

    switch (sock->state) {
    case SS_UNCONNECTED:
	/* This is ok... continue with connect */
	break;
    case SS_CONNECTED:
	/* Socket is already connected */
	return -EISCONN;
    case SS_CONNECTING:
	/* Not yet connected... we will check this. */

	/*
	 *      FIXME:  For all protocols what happens if you
	 *              start an async connect fork and both
	 *              children connect.
	 *
	 *              Clean this up in the protocols!
	 */

	break;
    default:
	return -EINVAL;
    }
    err = sock->ops->connect(sock, uservaddr, addrlen, file->f_flags);
    if (err < 0)
	return err;

    return 0;
}

int sock_register(int family, register struct proto_ops *ops)
{
    register struct proto_ops **props;

    for (props = pops; props < &pops[NPROTO]; props++)
	if (*props == NULL) {
	    *props = ops;
	    ops->family = family;
	    return (props - pops)/sizeof(struct proto_ops *);
	}
    return -ENOMEM;
}

void INITPROC sock_init(void)
{
    register struct net_proto *pro;

#ifndef CONFIG_SMALL_KERNEL
    printk("ELKS network sockets\n");
#endif

    /* Kick all configured protocols. */
    pro = protocols;
    while (pro->name != NULL) {
	(*pro->init_func) (pro);
	pro++;
    }
    /* We're all done... */
}

int sys_socket(int family, int type, int protocol)
{
    register struct socket *sock;
    register struct proto_ops *ops;
    int fd;

    ops = find_protocol_family(family);	/* Initially pops is not an array. */
    if (ops == NULL)
	return -EINVAL;

    if (type != SOCK_STREAM)
	return -EINVAL;

    if (!(sock = sock_alloc()))
	return -ENOSR;

    sock->type = (short int) type;
    sock->ops = ops;
    if ((fd = sock->ops->create(sock, protocol)) < 0) {
	sock_release(sock);
	return fd;
    }
    debug_net("NET(%d) new socket\n", current->pid);

    if ((fd = get_fd(SOCK_INODE(sock))) < 0) {
	sock_release(sock);
	return -EINVAL;
    }

    sock->file = current->files.fd[fd];

    return fd;
}

int sys_setsockopt(int fd, int level, int option_name, void *option_value,
	unsigned int option_len)
{
    register struct socket *sock;
    int err;
    struct linger l;

    if (!(sock = sockfd_lookup(fd, NULL)))
	return -ENOTSOCK;

    if (option_name != SO_LINGER || option_len != sizeof(struct linger))
	return -EINVAL;

    err = check_addr_to_kernel(option_value, sizeof(struct linger));
    if (err < 0)
	return err;
    memcpy_fromfs(&l, option_value, sizeof(struct linger));

    if (l.l_onoff != 0 && l.l_linger == 0)
	sock->flags |= SO_RST_ON_CLOSE;
    else sock->flags &= ~SO_RST_ON_CLOSE;
    printk("setsockopt reset %x\n", sock->flags);

    return 0;
}
#endif /* CONFIG_SOCKET */
