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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linuxmt/termios.h>

#include "ip.h"
#include "slip.h"
#include "vjhc.h"

/*#define DEBUG*/

#define SERIAL_BUFFER_SIZE	256

/*
 * SLIP codes
 */
#define END		0300	/* 0xC0*/
#define ESC		0333	/* 0xDB*/
#define ESC_END		0334	/* 0xDC*/
#define ESC_ESC		0335	/* 0xDD*/

static unsigned char 	sbuf[SERIAL_BUFFER_SIZE];
static unsigned char	lastchar;
static unsigned char 	packet[SLIP_MTU + 128];
static unsigned int	packpos;
static int devfd;

static speed_t convert_baudrate(speed_t baudrate)
{
	switch (baudrate) {
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
#ifdef B230400
	case 230400: return B230400;
#endif
#ifdef B460800
	case 460800: return B460800;
#endif
#ifdef B500000
	case 500000: return B500000;
#endif
#ifdef B576000
	case 576000: return B576000;
#endif
#ifdef B921600
	case 921600: return B921600;
#endif
#ifdef B1000000
	case 1000000: return B1000000;
#endif
	default:
		printf("Unknown baud rate %lu\n", (unsigned long)baudrate);
		return -1;
	}
}

int slip_init(char *fdev, speed_t baudrate)
{
    speed_t baud = 0;
    struct termios tios;

    if (baudrate)
	baud = convert_baudrate(baudrate);
    if (baud == -1)
	return -1;

    devfd = open(fdev, O_RDWR | O_NONBLOCK | O_EXCL | O_NOCTTY);
    if (devfd < 0) {
	printf("ktcp: failed to open serial device %s\n",fdev);
	return -1;
    }

    /* Setup the tty
     */
    ioctl(devfd, TCGETS, &tios);
    tios.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE);
    tios.c_oflag &= ~ONLCR;
    if (baud)
	tios.c_cflag = baud;
    tios.c_cflag |= CS8 | CREAD;
    tios.c_cflag |= CLOCAL;	/* ignore modem control lines*/
    //tios.c_cflag |= CRTSCTS;	/* hw flow control*/
    //tios.c_cc[VMIN] = 255;	/* try for max 255 byte reads*/
    //tios.c_cc[VTIME] = 1;	/* 100ms intercharacter timeout*/
    tios.c_cc[VMIN] = 1;
    tios.c_cc[VTIME] = 0;
    ioctl(devfd, TCSETS, &tios);

    packpos = 128;
    lastchar = 0;

#ifdef CSLIP
    ip_vjhc_init();
#endif

    return devfd;
}

#ifdef CSLIP
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
 *  Called when we have new data waiting at the serial port
 */
void slip_process(void)
{
    size_t p_size;
    unsigned char *p;
    int i, len;

    len = read(devfd, sbuf, SERIAL_BUFFER_SIZE);
    if (len <= 0)
	return;
#ifdef DEBUG
{
printf("[%d]", len);
printf("{");
for (i=0; i<len; i++)
    printf("%2x,", sbuf[i]);
printf("}");
}
#endif
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
#ifdef CSLIP
			p = packet;
		        cslip_decompress(&p, &p_size);
#else
			p = packet + 128;
#endif
		        if (p_size > 0)
			    ip_recvpacket(p, p_size);

			/* Reset */
			packpos = 128;
			lastchar = 0;
			continue;

		    default:
			/* drop characters over SLIP_MTU*/
			if (packpos < sizeof(packet))
			    packet[packpos++] = sbuf[i];
		}
	    }
	    lastchar = sbuf[i];
	}
    }

#ifdef CSLIP
void cslip_compress(__u8 **packet, int *len)
{
    pkt_ut p;
    __u8 type;
    //int orig_len = len;

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

    //printf("cslip : from %d to %d\n", orig_len, p.p_size);
}
#endif

void slip_send(unsigned char *packet, int len)
{
    unsigned char buf[128+2];
    unsigned char *p = packet;
    unsigned char *q = buf;

#ifdef CSLIP
    cslip_compress(&p, &len);
#endif

    *q++ = END;
    while (len--) {
	switch (*p) {
	case END:
	    *q++ = ESC;
	    *q++ = ESC_END;
	    break;
	case ESC:
	    *q++ = ESC;
	    *q++ = ESC_ESC;
	    break;
	default:
	    *q++ = *p;
	    break;
	}
	p++;
	if (q - buf >= sizeof(buf) - 2)
	    write(devfd, buf, q - buf);
    }
    *q++ = END;
    write(devfd, buf, q - buf);
}
