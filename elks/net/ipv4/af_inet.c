#include <linuxmt/config.h>
#include <linuxmt/socket.h>
#include <linuxmt/net.h>

inet_create()
{
printk("inet_create\n");
return -1;
}


static int inet_dup()
{
printk("inet_dup\n");
}

inet_release()
{
printk("inet_release\n");
}

inet_bind()
{
printk("inet_bind\n");
}

inet_connect()
{
printk("inet_connect\n");
}

inet_socketpair()
{
printk("inet_sockpair\n");
}

inet_accept()
{
printk("inet_accept\n");
}

inet_getname()
{
printk("inet_getname\n");
}

inet_select()
{
printk("inet_select\n");
}

inet_ioctl()
{
printk("inet_ioctl\n");
}

inet_listen()
{
printk("inet_listen\n");
}

inet_shutdown()
{
printk("inet_shutdown\n");
}

inet_setsockopt()
{
printk("setsockopt\n");
}

inet_getsockopt()
{
printk("inet_getsockopt\n");
}

inet_fcntl()
{
printk("inet_fcntl\n");
}

inet_sendmsg()
{
printk("inet_sendmsg\n");
}

inet_recvmsg()
{
printk("inet_recvmsg\n");
}




static struct proto_ops inet_proto_ops = {
        AF_INET,

        inet_create,
        inet_dup,
        inet_release,
        inet_bind,
        inet_connect,
        inet_socketpair,
        inet_accept,
        inet_getname, 
        inet_select,
        inet_ioctl,
        inet_listen,
        inet_shutdown,
        inet_setsockopt,
        inet_getsockopt,
        inet_fcntl,
	inet_sendmsg,
	inet_recvmsg
};








void inet_proto_init(pro)
struct net_proto * pro;

{

	printk("ELKS TCP/IP Sockets - Not *Yet* Implemented\n");
	/*sock_register(AF_INET, &inet_proto_ops);*/
	sock_register(inet_proto_ops.family, &inet_proto_ops); 


};


