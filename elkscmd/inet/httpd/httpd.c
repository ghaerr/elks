/*
 * An nano-http server. Part of the linux-86 project
 * 
 * Copyright (c) 2001 Harry Kalogirou <harkal@rainbow.cs.unipi.gr>
 *
 */
 
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef __LINUX__
#include <linuxmt/in.h>
#include <linuxmt/net.h>
#include "netorder.h"
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#define DEF_PORT		80
#define DEF_DOCBASE	"/usr/lib/httpd"
#define DEF_CONTENT	"text/html"

#define WS(c)	( ((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n') )

int listen_sock;
int conn_sock;
#define BUF_SIZE	2048
char buf[BUF_SIZE];
char doc_base[64];

#if 1
char* get_mime_type(name)
char* name;
{
    char* dot;

    dot = strrchr( name, '.' );
    if ( dot == (char*) 0 )
	return "text/plain";
    if ( strcmp( dot, ".html" ) == 0 || strcmp( dot, ".htm" ) == 0 )
	return "text/html";
    if ( strcmp( dot, ".jpg" ) == 0 || strcmp( dot, ".jpeg" ) == 0 )
	return "image/jpeg";
    if ( strcmp( dot, ".gif" ) == 0 )
	return "image/gif";
    if ( strcmp( dot, ".png" ) == 0 )
	return "image/png";
    if ( strcmp( dot, ".css" ) == 0 )
	return "text/css";
    if ( strcmp( dot, ".vrml" ) == 0 || strcmp( dot, ".wrl" ) == 0 )
	return "model/vrml";
    if ( strcmp( dot, ".midi" ) == 0 || strcmp( dot, ".mid" ) == 0 )
	return "audio/midi";
    return "text/plain";
}
#endif

void send_header(ct)
char *ct;
{
	buf[0] = 0;
	sprintf(buf, "HTTP/1.0 200 OK\r\nServer: nanoHTTPd/0.1\r\nDate: Thu Apr 26 15:37:46 GMT 2001\r\nContent-Type: %s\r\n",ct);
	write(conn_sock, buf, strlen(buf));
}

void send_error(errnum, str)
int	errnum;
char *str;
{
	buf[0] = 0;
	sprintf(buf,"HTTP/1.0 %d %s\r\nContent-type: %s\r\n", errnum, str, DEF_CONTENT);
	write(conn_sock, buf, strlen(buf));
	buf[0] = 0;
	sprintf(buf,"Connection: close\r\nDate: Thu Apr 26 15:37:46 GMT 2001\r\n\r\n%s\r\n",str);
	write(conn_sock, buf, strlen(buf));
}

void process_request()
{
	int p;
	int fin, size;
	int ret;
	char *c, *file, fullpath[128];
	struct stat st;
	
	ret = read(conn_sock, buf, BUF_SIZE);
	
	c = buf;
	while (*c && !WS(*c) && c < (buf + sizeof(buf))){
		c++;
	}
	*c = 0;
	
	if (strcasecmp(buf, "get")){
		send_error(404, "Method not supported");
		return;		
	}
	
	file = ++c;
	while (*c && !WS(*c) && c < (buf + sizeof(buf))){
		c++;
	}
	*c = 0;

	/* TODO : Use strncat when security is the only problem of this server! */
	strcpy(fullpath, doc_base);
	strcat(fullpath, file);
	stat(fullpath, &st);
	
	if ((st.st_mode & S_IFMT) == S_IFDIR){
		if (file[strlen(fullpath) - 1] != '/'){
			strcat(fullpath, "/");
		}
		strcat(fullpath, "index.html");
	}
	
	fin = open(fullpath, O_RDONLY);
	if (fin < 0){
		send_error(404, "Document (probably) not found");
		return;
	}
	size = lseek(fin, (off_t)0, SEEK_END);
	lseek(fin, (off_t)0, SEEK_SET);
	send_header(get_mime_type(fullpath));
	buf[0] = 0;
	sprintf(buf,"Content-Length: %d\r\n\r\n",size);
	write(conn_sock, buf, strlen(buf));
	
	do {
		ret = read(fin, buf, BUF_SIZE);
		write(conn_sock, buf, ret);
	} while (ret == BUF_SIZE);
	
	close(fin);
	
}

int main(argc, argv)
int argc;
char** argv;
{
	int ret;
	unsigned long i;
	char *t;
	struct sockaddr_in localadr;

	ret = fork();
	if (ret > 0 || ret == -1){
		exit(0);	
	}
	ret = open("/dev/null", O_RDWR); /* our log file! */
	dup2(ret, 0);
	dup2(ret, 1);
	dup2(ret, 2);
	close(ret);
	setsid();

	strcpy(doc_base, DEF_DOCBASE);
	
	listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	
	localadr.sin_family = AF_INET;
	localadr.sin_port = htons(DEF_PORT);
	localadr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(	listen_sock,
    			(struct sockaddr *)&localadr,
    			sizeof(struct sockaddr_in));

	ret = listen(listen_sock, 5);

	while (1){
		conn_sock = accept(listen_sock, NULL, NULL);
		
		if (conn_sock < 0)
			continue;

		ret = fork();
		if (ret == 0){
			close(listen_sock);
			process_request();
			close(conn_sock);
			exit(0);
		} else {
			close(conn_sock);
		}
	}
}

