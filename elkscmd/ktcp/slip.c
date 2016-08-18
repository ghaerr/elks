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

#include "config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linuxmt/termios.h>

#include "slip.h"
#include "vjhc.h"

/*#define DEBUG*/

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
static unsigned char 	packet[SLIP_MTU + 128];
static unsigned int	packpos;
static int devfd;

int slip_init(char *fdev)
{
    int len;
    struct termios tios;

    devfd = open(fdev, O_NONBLOCK|O_RDWR);
    if (devfd < 0)
	printf("ERROR : failed to open serial device %s\n",fdev);

    /* Setup the tty
     */
    ioctl(devfd, TCGETS, &tios);
    tios.c_lflag &= ~(ICANON|ECHO);
    tios.c_oflag &= ~ONLCR;

    ioctl(devfd, TCSETS, &tios);

    /* Init some variables 
     */
    packpos = 128;
    lastchar = 0;

#ifdef CONFIG_CSLIP
    ip_vjhc_init();
#endif

    return devfd;
}

#ifdef CONFIG_CSLIP
void cslip_decompress(__u8 **packet, size_t *len){
    pkt_ut p;
    __u8 c;
    
    p.p_data = *packet;
    p.p_size = *len;
    p.p_offset = 128;
    p.p_maxsize = SLIP_MTU;
    
    c = *(*packet + 128) & 0xf0;
    if ( c != TYPE_IP){   	
        if (c & 0x80){
            c = TYPE_COMPRESSED_TCP;
            ip_vjhc_arr_compr(&p);
#ifdef DEBUG
            printf("CSLIP : Compressed TCP packet offset %d size %d (%d)\n", p.p_offset, p.p_size, *len);
#endif
        }
        else if (c == TYPE_UNCOMPRESSED_TCP){
            *(*packet + 128) &= 0x4f;
            ip_vjhc_arr_uncompr(&p);
#ifdef DEBUG
            printf("CSLIP : Uncompressed TCP packet offset %d size %d (%d)\n", p.p_offset, p.p_size, *len);
#endif
        }
        if ((p.p_size > 0)){
            *packet += p.p_offset;
        }
    } else {
#ifdef DEBUG
        printf("CSLIP : IP packet\n");
#endif
    	*packet += 128;
    }
    
    *len = p.p_size;
}
#endif

/*
 * slip_process()
 *  Called when we have new data waiting 
 *  at the serial port
 *	FIXME : Handle the buffer overrun when out
 *          MTU is not honored.
 */
void slip_process(void)
{
    size_t p_size;
    __u8 *p;
    int i, len, packet_num = 0;

    while (packet_num < 3) {
	len = read(devfd, &sbuf, SERIAL_BUFFER_SIZE);
	if (len <= 0)
	    break;

	for (i=0; i < len ; i++) {
	    if (lastchar == ESC) {
		switch (sbuf[i]) { 

		    case ESC_END:
			packet[packpos++] = END; 
			break;

		    case ESC_ESC:
			packet[packpos++] = ESC;
			break;

		    default:

			/* Protocol error ??! */
			packet[packpos++] = sbuf[i];
		}
	    } else {
		switch (sbuf[i]) {

		    case ESC:
			break;

		    case END:
			if (packpos == 128)
			    break;

			p_size = packpos - 128;
#ifdef CONFIG_CSLIP			
			p = packet;
		        cslip_decompress(&p, &p_size);		        
#else
			p = packet + 128;
#endif
		        if (p_size > 0)
			    ip_recvpacket(p, p_size);

			packet_num++;

			/* Reset */
			packpos = 128;
			lastchar = 0;
			break;

		    default:
			packet[packpos++] = sbuf[i];
		}
	    }
	    lastchar = sbuf[i];
	}
    }
}

void send_char(__u8 ch)
{
    int i = write(devfd, &ch, 1);

#if 0
    if (i <= 0)
	perror("skata");
#endif
}

#ifdef CONFIG_CSLIP
void cslip_compress(__u8 **packet, int *len)
{
    pkt_ut p;
    __u8 type;
    size_t orig_len;
    
    orig_len = *len;
    
    p.p_data = *packet;
    p.p_size = *len;
    p.p_offset = 0;
    p.p_maxsize = SLIP_MTU;
    
    type = ip_vjhc_compress(&p);
    
    if (type != TYPE_IP){
        *packet += p.p_offset;
        *len = p.p_size;
        *packet[0] |= type;
    }

#ifdef DEBUG   
    printf("cslip : from %d to %d\n", orig_len, p.p_size);
#endif
}
#endif

/*
 * TODO : Use a buffer to reduse the write calls
 */
void slip_send(char *packet, int len)
{
    __u8 *p = (__u8 *)packet;
    
#ifdef CONFIG_CSLIP
    cslip_compress(&p, &len);
#endif

    send_char(END);
    while (len--) {
	switch (*p) {

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
		break;

	}
	p++;
    }
    send_char(END);
}



