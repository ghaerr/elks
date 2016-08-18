/*
ttn.c
*/

/*
 * Telnet for ELKS 
 *
 * Based on minix telnet client.
 * (c) 2001 Harry Kalogirou <harkal@rainbow.cs.unipi.gr>
 *
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef __LINUX__
#include <linuxmt/in.h>
#include <linuxmt/net.h>
#include <linuxmt/time.h>
#include "../httpd/netorder.h"

/*#define assert(a)*/
#else
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/*#include <netdb.h>*/

#include "ttn.h"

/*#define DEBUG 1*/

#if DEBUG
#define where() (fprintf(stderr, "%s %d:", __FILE__, __LINE__))
#endif

static char *prog_name;
static int tcp_fd;
static char *term_env;
static char done;
static struct termios def_termios;

void finish()
{
	tcsetattr(0, TCSANOW, &def_termios);
	exit(0);	
}

unsigned long int in_aton(str)
const char *str;
{
        unsigned long l;
        unsigned int val;
        int i;

        l = 0;
        for (i = 0; i < 4; i++)
        {
                l <<= 8;
                if (*str != '\0')
                {
                        val = 0;
                        while (*str != '\0' && *str != '.')
                        {
                                val *= 10;
                                val += *str - '0';
                                str++;
                        }
                        l |= val;
                        if (*str != '\0')
                                str++;
                }
        }
        return(htonl(l));   
}


static void keybd()
{
	char buffer[1024];
	int result;
	int count;

		count= read (0, buffer, sizeof(buffer));
		if (!count)
			return;
#if DEBUG && 0
 { where(); fprintf(stderr, "writing %d bytes\r\n", count); }
#endif
		count = write(tcp_fd, buffer, count);
		if (count<0)
		{
			perror("Connection closed");
			finish();
		}
}

static void scrn()
{
	char buffer[1024], *bp, *iacptr;
	int count, optsize;

		count = read (tcp_fd, buffer, sizeof(buffer));
#if DEBUG && 0
 { where(); fprintf(stderr, "read %d bytes\r\n", count); }
#endif
		if (count < 0)
		{
			perror ("Connection closed");
			finish();
		}
		if (!count)
			return;
		bp= buffer;
		do
		{
			iacptr= memchr (bp, IAC, count);
			if (!iacptr)
			{
				write(1, bp, count);
				count= 0;
			}
			if (iacptr && iacptr>bp)
			{
#if DEBUG
 { where(); fprintf(stderr, " ptr-bp= %d\r\n", iacptr-bp); }
#endif
				write(1, bp, iacptr-bp);
				count -= (iacptr-bp);
				bp= iacptr;
				continue;
			}
			if (iacptr)
			{
				optsize= process_opt(bp, count);
#if DEBUG && 0
 { where(); fprintf(stderr, "process_opt(...)= %d\r\n", optsize); }
#endif
				if (optsize<0)
					return;
assert (optsize);
				bp += optsize;
				count -= optsize;
			}
		} while (count);
}



int
main(argc, argv)
int argc;
char *argv[];
{
	long host;
	unsigned short port;
	int pid, ppid;
	int result, count;
	char buffer[1024];
	struct sockaddr_in locadr, remadr;
	char *tcp_device;
	int ret;
	short nonblock;
	fd_set fdset;

	prog_name= argv[0];
	if (argc <2 || argc>3)
	{
		fprintf(stderr, "Usage: %s host <port>\r\n", argv[0]);
		exit(1);
	}

	tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
	
	locadr.sin_family = AF_INET;
	locadr.sin_port = 0; /* any port */
	locadr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(	tcp_fd,
    			(struct sockaddr *)&locadr,
    			sizeof(struct sockaddr_in));
	if (ret < 0){
		perror("Bind failed");
		exit(1);
	}

	if (argc == 3){
		sscanf(argv[2],"%d", &port);
	} else {
		port = 23;	
	}
		
	remadr.sin_family = AF_INET;
	remadr.sin_port = htons( port );
	remadr.sin_addr.s_addr = in_aton(argv[1]);

	printf("Connecting to %s port %u\n",argv[1], port);
	ret = connect(tcp_fd, (struct sockaddr *)&remadr,
    			sizeof(struct sockaddr_in));
	if (ret < 0){
		perror("Connection failed");
		exit(1);
	}
	printf("Connected\r\n");

	tcgetattr(0, &def_termios);
	
	nonblock = 1;
	ioctl(0, FIONBIO, &nonblock);

	for (;;){
		FD_ZERO(&fdset);
        FD_SET(0, &fdset);
        FD_SET(tcp_fd, &fdset);

		select(tcp_fd + 1, &fdset, NULL, NULL, NULL);

		if (FD_ISSET(tcp_fd, &fdset)){
			scrn();
		}

		if (FD_ISSET(0, &fdset)){
			keybd();
		}			
	}
}

#define next_char(var) \
	if (offset<count) { (var) = bp[offset++]; } \
	else { read(tcp_fd, (char *)&(var), 1); }

static void do_option (optsrt)
int optsrt;
{
	unsigned char reply[3], *rp;
	int count, result;

	switch (optsrt)
	{
	case OPT_TERMTYPE:
		if (WILL_terminal_type)
			return;
		if (!WILL_terminal_type_allowed)
		{
			reply[0]= IAC;
			reply[1]= IAC_WONT;
			reply[2]= optsrt;
		}
		else
		{
			WILL_terminal_type= TRUE;
			term_env= getenv("TERM");
			if (!term_env)
				term_env= "unknown";
			reply[0]= IAC;
			reply[1]= IAC_WILL;
			reply[2]= optsrt;
		}
		break;
	default:
#if DEBUG
 { where(); fprintf(stderr, "got a DO (%d)\r\n", optsrt); }
#endif
#if DEBUG
 { where(); fprintf(stderr, "WONT (%d)\r\n", optsrt); }
#endif
		reply[0]= IAC;
		reply[1]= IAC_WONT;
		reply[2]= optsrt;
		break;
	}
	result= writeall(tcp_fd, (char *)reply, 3);
	if (result<0)
		perror("write");
}

static void will_option (optsrt)
int optsrt;
{
	unsigned char reply[3], *rp;
	int count, result;

	switch (optsrt)
	{
	case OPT_ECHO:
		if (DO_echo)
			break;
		if (!DO_echo_allowed)
		{
			reply[0]= IAC;
			reply[1]= IAC_DONT;
			reply[2]= optsrt;
		}
		else
		{
			struct termios termios;
			tcgetattr(0, &termios);
			termios.c_iflag &= ~(ICRNL|IGNCR|INLCR|IXON|IXOFF);
			termios.c_oflag &= ~(OPOST);
			termios.c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG);
			tcsetattr(0, TCSANOW, &termios);
			DO_echo= TRUE;
			reply[0]= IAC;
			reply[1]= IAC_DO;
			reply[2]= optsrt;
		}
		result= writeall(tcp_fd, (char *)reply, 3);
		if (result<0)
			perror("write");
		break;
	case OPT_SUPP_GA:
		if (DO_suppress_go_ahead)
			break;
		if (!DO_suppress_go_ahead_allowed)
		{
			reply[0]= IAC;
			reply[1]= IAC_DONT;
			reply[2]= optsrt;
		}
		else
		{
			DO_suppress_go_ahead= TRUE;
			reply[0]= IAC;
			reply[1]= IAC_DO;
			reply[2]= optsrt;
		}
		result= writeall(tcp_fd, (char *)reply, 3);
		if (result<0)
			perror("write");
		break;
	default:
#if DEBUG
 { where(); fprintf(stderr, "got a WILL (%d)\r\n", optsrt); }
#endif
#if DEBUG
 { where(); fprintf(stderr, "DONT (%d)\r\n", optsrt); }
#endif
		reply[0]= IAC;
		reply[1]= IAC_DONT;
		reply[2]= optsrt;
		result= writeall(tcp_fd, (char *)reply, 3);
		if (result<0)
			perror("write");
		break;
	}
}

static int writeall (fd, buffer, buf_size)
int fd;
char *buffer;
int buf_size;
{
	int result;

	while (buf_size)
	{
		result= write (fd, buffer, buf_size);
		if (result <= 0)
			return -1;
assert (result <= buf_size);
		buffer += result;
		buf_size -= result;
	}
	return 0;
}

static void dont_option (optsrt)
int optsrt;
{
	unsigned char reply[3], *rp;
	int count, result;

	switch (optsrt)
	{
	default:
#if DEBUG
 { where(); fprintf(stderr, "got a DONT (%d)\r\n", optsrt); }
#endif
		break;
	}
}

static void wont_option (optsrt)
int optsrt;
{
	unsigned char reply[3], *rp;
	int count, result;

	switch (optsrt)
	{
	default:
#if DEBUG
 { where(); fprintf(stderr, "got a WONT (%d)\r\n", optsrt); }
#endif
		break;
	}
}

static int sb_termtype (bp, count)
char *bp;
int count;
{
	unsigned char command, iac, optsrt;
	unsigned char buffer[4];
	int offset, result, ret_value;

	offset= 0;
	next_char(command);
	if (command == TERMTYPE_SEND)
	{
		buffer[0]= IAC;
		buffer[1]= IAC_SB;
		buffer[2]= OPT_TERMTYPE;
		buffer[3]= TERMTYPE_IS;
		result= writeall(tcp_fd, (char *)buffer,4);
		if (result<0){
			ret_value = result;
			goto ret;
		}
		count= strlen(term_env);
		if (!count)
		{
			term_env= "unknown";
			count= strlen(term_env);
		}
		result= writeall(tcp_fd, term_env, count);
		if (result<0){
			ret_value = result;
			goto ret;
		}
		buffer[0]= IAC;
		buffer[1]= IAC_SE;
		result= writeall(tcp_fd, (char *)buffer,2);
		if (result<0){
			ret_value = result;
			goto ret;			
		}

	}
	else
	{
#if DEBUG
 where();
#endif
		fprintf(stderr, "got an unknown command (skipping)\r\n");
	}
	for (;;)
	{
		next_char(iac);
		if (iac != IAC)
			continue;
		next_char(optsrt);
		if (optsrt == IAC)
			continue;
		if (optsrt != IAC_SE)
		{
#if DEBUG
 where();
#endif
		/*	fprintf(stderr, "got IAC %d\r\n", optsrt);*/
		}
		break;
	}	
	ret_value = offset;
ret:
	return ret_value;
}


static int process_opt (bp, count)
char *bp;
int count;
{
	unsigned char iac, command, optsrt, sb_command;
	int offset, result;	;
#if DEBUG && 0
 { where(); fprintf(stderr, "process_opt(bp= 0x%x, count= %d)\r\n",
	bp, count); }
#endif

	offset= 0;
assert (count);
	next_char(iac);
assert (iac == IAC);
	next_char(command);
	switch(command)
	{
	case IAC_NOP:
		break;
	case IAC_DataMark:
fprintf(stderr, "got a DataMark\r\n");
		break;
	case IAC_BRK:
fprintf(stderr, "got a BRK\r\n");
		break;
	case IAC_IP:
fprintf(stderr, "got a IP\r\n");
		break;
	case IAC_AO:
fprintf(stderr, "got a AO\r\n");
		break;
	case IAC_AYT:
fprintf(stderr, "got a AYT\r\n");
		break;
	case IAC_EC:
fprintf(stderr, "got a EC\r\n");
		break;
	case IAC_EL:
fprintf(stderr, "got a EL\r\n");
		break;
	case IAC_GA:
fprintf(stderr, "got a GA\r\n");
		break;
	case IAC_SB:
		next_char(sb_command);
		switch (sb_command)
		{
		case OPT_TERMTYPE:
#if DEBUG && 0
fprintf(stderr, "got SB TERMINAL-TYPE\r\n");
#endif
			result= sb_termtype(bp+offset, count-offset);
			if (result<0)
				return result;
			else
				return result+offset;
		default:
fprintf(stderr, "got an unknown SB (skiping)\r\n");
			for (;;)
			{
				next_char(iac);
				if (iac != IAC)
					continue;
				next_char(optsrt);
				if (optsrt == IAC)
					continue;
if (optsrt != IAC_SE)
	fprintf(stderr, "got IAC %d\r\n", optsrt);
				break;
			}
		}
		break;
	case IAC_WILL:
		next_char(optsrt);
		will_option(optsrt);
		break;
	case IAC_WONT:
		next_char(optsrt);
		wont_option(optsrt);
		break;
	case IAC_DO:
		next_char(optsrt);
		do_option(optsrt);
		break;
	case IAC_DONT:
		next_char(optsrt);
		dont_option(optsrt);
		break;
	case IAC:
/*fprintf(stderr, "got a IAC\r\n");*/
		break;
	default:
fprintf(stderr, "got unknown command (%d)\r\n", command);
	}
	return offset;
}

