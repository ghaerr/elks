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

#include <sys/types.h>

#include <sys/stat.h>
#include <fcntl.h>
#include "slip.h"
#include <linuxmt/termios.h>

#define SERIAL_BUFFER_SIZE	256

/*
 * SLIP codes
 */
#define END		0300
#define ESC		0333
#define ESC_END		0334
#define ESC_ESC		0335


static unsigned char 	sbuf[SERIAL_BUFFER_SIZE];
static unsigned char	lastchar;
static unsigned char 	packet[512];
static unsigned int	packpos;
static int devfd;


int slip_init(fdev)
char *fdev;
{
    int len;
    struct termios tios;

    devfd = open(fdev, O_NONBLOCK|O_RDWR);
    if(devfd < 0){
		printf("ERROR : failed to open serial device %s\n",fdev);
    }
    
    /* Setup the tty
     */
    ioctl(devfd, TCGETS, &tios);
    tios.c_lflag &= ~(ICANON|ECHO);
    tios.c_oflag &= ~ONLCR;
/*	tios.c_cflag &= ~017;
	tios.c_cflag |= 0xd;*/
    ioctl(devfd, TCSETS, &tios);
    
    /* Init some variables 
     */
    packpos = 0;
    lastchar = 0;
    
    return devfd;
    
}

/*
 * slip_process()
 *  Called when we have new data waiting 
 *  at the serial port
 */
void slip_process()
{
    int i, len;
    int packet_num = 0;

    while(packet_num < 3){
	len = read(devfd, &sbuf, SERIAL_BUFFER_SIZE);
	if(len <= 0)break;

	for( i = 0 ; i < len ; i++ ){
	    if(lastchar == ESC){
		switch (sbuf[i]){    
		case ESC_END:
		    packet[packpos++] = END; 
		    break;
		case ESC_ESC:
		    packet[packpos++] = ESC;
		    break;
		default:
		    /* Protocol error ??! */
		    packet[packpos++] = sbuf[i];
		} /* switch */
	    } 
	    else {
		switch (sbuf[i]){
		case ESC:
		    break;
		    
		case END:
		    if(packpos == 0)
			break;

		    ip_recvpacket(&packet, packpos);
		    packet_num++;
		    /* Reset */
		    packpos = 0;
		    lastchar = 0;
		    break;
		default:
		    packet[packpos++] = sbuf[i];
		} /* switch */	
	    }
	    lastchar = sbuf[i];
	} /* for */
	
		
    }
}

void send_char(ch)
__u8 ch;
{
    int i;
    i = write(devfd, &ch, 1);
/*    if(i <= 0){
		perror("skata");
    }*/
}

/*
 * TODO : Use a buffer to reduse the write calls
 */
void slip_send(packet, len)
char *packet;
int len;
{
    __u8 *p;
    
    p = (__u8 *)packet;
    send_char(END);
    
    while(len--){
	switch(*p){
	case END:
	    send_char(ESC);
	    send_char(ESC_END);
	    break;
	case ESC:
	    send_char(ESC);
	    send_char(ESC_ESC);
	    break;
	default:
	    send_char(*p);
	}
	p++;
    }   
    
    send_char(END); 
}

