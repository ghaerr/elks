/*
 * This file is part of the ELKS TCP/IP stack
 *
 * (C) 2001 Harry Kalogirou (harkal@rainbow.cs.unipi.gr)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */ 

#include <linuxmt/time.h>
#include <sys/types.h>
#include <unistd.h> 
#include "slip.h"
#include "tcpdev.h"
#include "timer.h"
#include "mylib.h"
#include "ip.h"

extern int tcp_timeruse;

static int sfd;
static int tcpdevfd;

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

void ktcp_run()
{
    fd_set fdset;
    struct timeval timeint, *tv;

    while(1){
    	if(tcp_timeruse > 0){
	    	timeint.tv_sec  = 1;
    		timeint.tv_usec = 0;
    		tv = &timeint;
    	} else {
    		tv = NULL;
    	}
    		
        FD_ZERO(&fdset);
        FD_SET(sfd, &fdset);
        FD_SET(tcpdevfd, &fdset);

		select(sfd > tcpdevfd ? sfd + 1 : tcpdevfd + 1, &fdset, NULL, NULL, tv);
		
		Now = timer_get_time();
	
		if(tcp_timeruse > 0)
			tcp_update();
			
		if(FD_ISSET(sfd, &fdset)){
	    	slip_process();
		}
	
		if(FD_ISSET(tcpdevfd, &fdset)){
	    	tcpdev_process();
		}

		if(tcp_timeruse > 0)		
			tcp_retrans();
	
		/*   tcpcb_printall();*/
    }
}

int main(argc, argv)
int argc;
char **argv;
{

    if(argc != 3){
		printf("Syntax :\n    %s local_ip slip_tty\n",argv[0]);
		exit(3);
    }

    local_ip = in_aton(argv[1]);
    
    if((tcpdevfd = tcpdev_init("/dev/tcpdev")) < 0){
		exit(1);
    }

    if((sfd = slip_init(argv[2])) < 0){
		exit(2);
    }

    ip_init();
    icmp_init();
    tcp_init();
    
    ktcp_run(); 
    
    return 0;       
}


