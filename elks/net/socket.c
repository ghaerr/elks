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
#include <arch/irq.h>

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
    static struct socket ini_sock = {	/* order dependent on net.h! */
	SS_UNCONNECTED, /* state */
	NULL,		/* wait */
	0,		/* flags */
	0,		/* rcv_bufsiz */
	NULL,		/* ops */
	NULL,		/* inode */
	NULL,		/* file */

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO)
	NULL,		/* conn */
	NULL,		/* iconn */
	NULL,		/* next */
	NULL,		/* data */
#endif
#if defined(CONFIG_INET)
	0,		/* sem */
	0,		/* avail_data */
	0,		/* retval */
	0,		/* remaddr */
	0,		/* localaddr */
	0,		/* remport */
	0		/* localport */
#endif
    };
    register struct inode *inode;
    register struct socket *sock;

    if (!(inode = new_inode(NULL, S_IFSOCK)))
	return NULL;

    sock = &inode->u.socket_i;

    *sock = ini_sock;

    sock->wait = (struct wait_queue *)inode; /* sockets wait on same wq as inodes locks */
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
	debug_net("NET(%P) can't find sock_read socket\n");
	return -EBADF;
    }

    if (sock->flags & SF_ACCEPTCON)
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
	debug_net("NET(%P) can't find sock_write socket\n");
	return -EBADF;
    }

    if (sock->flags & SF_ACCEPTCON)
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

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO)
int sock_awaitconn(register struct socket *mysock, struct socket *servsock, int flags)
{
    register struct socket *last;

    /*
     *      We must be listening
     */
    if (!(servsock->flags & SF_ACCEPTCON))
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

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO)
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

#if defined(CONFIG_UNIX) || defined(CONFIG_NANO)
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

static struct file_operations socket_file_ops = {
    NULL,			/* lseek */
    sock_read,			/* read */
    sock_write,			/* write */
    NULL,			/* readdir */
    sock_select,		/* select */
    NULL,			/* ioctl */
    NULL,			/* open */
    sock_close			/* close */
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
    NULL,			/* getblk */
    NULL			/* truncate */
};

static int get_fd(register struct inode *inode)
{
    int fd;

    if ((fd = open_fd(O_RDWR, inode)) >= 0)
	inode->i_count++;		/*FIXME: Really needed?*/
    return fd;
}

int sys_listen(int fd, int backlog)
{
    register struct socket *sock;
    register struct proto_ops *ops;

    if (!(sock = sockfd_lookup(fd, NULL)))
	return -ENOTSOCK;

    if (sock->state != SS_UNCONNECTED)
	return -EINVAL;

    debug_net("NET(%P) sys_listen sock %x\n", sock);
    ops = sock->ops;
    if (ops && ops->listen)
	ops->listen(sock, backlog);

    sock->flags |= SF_ACCEPTCON;
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

    if (!(sock->flags & SF_ACCEPTCON))
	return -EINVAL;

    if (!(newsock = sock_alloc())) {
	debug_net("NET(%P) sys_accept: no more sockets\n");
	return -ENOSR;		/* Was EAGAIN, but we are out of system resources! */
    }

    //newsock->type = sock->type;
    newsock->ops = sock->ops;
    if ((i = sock->ops->dup(newsock, sock)) < 0) {
	sock_release(newsock);
	return i;
    }

    debug_tune("(%P) before accept sock %x newsock %x\n", sock, newsock);
    i = newsock->ops->accept(sock, newsock, file->f_flags);
    if (i < 0) {
	sock_release(newsock);
	return i;
    }
    debug_tune("(%P) after accept sock %x newsock %x\n", sock, newsock);

    if ((fd = get_fd(SOCK_INODE(newsock))) < 0) {
	sock_release(newsock);
	return -EINVAL;
    }

    if (upeer_sockaddr)
	newsock->ops->getname(newsock, upeer_sockaddr, upeer_addrlen, 1);

    return fd;
}

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

    /* Kick all configured protocols. */
    pro = protocols;
    while (pro->name != NULL) {
	(*pro->init_func) (pro);
	pro++;
    }
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

    //sock->type = type;
    sock->ops = ops;
    if ((fd = sock->ops->create(sock, protocol)) < 0) {
	sock_release(sock);
	return fd;
    }
    debug_net("NET(%P) new socket\n");

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
    int flags;
    int setoption = 0;
    struct linger l;

    if (!(sock = sockfd_lookup(fd, NULL)))
	return -ENOTSOCK;

    flags = check_addr_to_kernel(option_value, option_len);
    if (flags < 0)
	return flags;

    switch (option_name) {
    case SO_LINGER:
	if (option_len != sizeof(struct linger))
	    return -EINVAL;
	memcpy_fromfs(&l, option_value, sizeof(struct linger));
	flags = SF_RST_ON_CLOSE;
	if (l.l_onoff != 0 && l.l_linger == 0)
	    setoption = 1;
	break;

    case SO_REUSEADDR:
    case SO_RCVBUF:
	if (option_len != sizeof(int))
	    return -EINVAL;
	memcpy_fromfs(&setoption, option_value, sizeof(int));
	if (option_name == SO_RCVBUF) {
	    sock->rcv_bufsiz = setoption;
	    return 0;
	}
	flags = SF_REUSE_ADDR;
	break;

    default:
	return -EINVAL;
    }

    if (setoption)
	sock->flags |= flags;
    else sock->flags &= ~flags;

    return 0;
}

int sys_getsocknam(int fd, struct sockaddr *usockaddr, int *usockaddr_len, int peer)
{
    struct socket *sock;

    if (!(sock = sockfd_lookup(fd, NULL)))
	return -ENOTSOCK;

    if (!usockaddr)
	return -EINVAL;

    return sock->ops->getname(sock, usockaddr, usockaddr_len, peer);
}

#endif /* CONFIG_SOCKET */
