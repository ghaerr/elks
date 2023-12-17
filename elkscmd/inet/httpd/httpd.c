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

#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <paths.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define DEF_PORT		80
#define DEF_CONTENT	"text/html"

#define WS(c)	( ((c) == ' ') || ((c) == '\t') || ((c) == '\r') || ((c) == '\n') )

int listen_sock;
char buf[1536];

char* get_mime_type(char *name)
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

void send_header(int fd, char *ct)
{
	buf[0] = 0;
	sprintf(buf, "HTTP/1.0 200 OK\r\nServer: nanoHTTPd/0.1\r\nDate: Thu Apr 26 15:37:46 GMT 2001\r\nContent-Type: %s\r\n",ct);
	write(fd, buf, strlen(buf));
}

void send_error(int fd, int errnum, char *str)
{
	sprintf(buf,"HTTP/1.0 %d %s\r\nContent-type: %s\r\n", errnum, str, DEF_CONTENT);
	write(fd, buf, strlen(buf));
	sprintf(buf,"Connection: close\r\nDate: Thu Apr 26 15:37:46 GMT 2001\r\n\r\n%s\r\n",str);
	write(fd, buf, strlen(buf));
}

void process_request(int fd)
{
	int fin, ret;
	off_t size;
	char *c, *file, fullpath[PATH_MAX];
	struct stat st;
	
	ret = read(fd, buf, sizeof(buf));
	
	c = buf;
	while (*c && !WS(*c) && c < (buf + sizeof(buf))){
		c++;
	}
	*c = 0;
	
	if (strcasecmp(buf, "get")){
		send_error(fd, 404, "Method not supported");
		return;		
	}
	
	file = ++c;
	while (*c && !WS(*c) && c < (buf + sizeof(buf))){
		c++;
	}
	*c = 0;

	/* TODO : Use strncat when security is the only problem of this server! */
	strcpy(fullpath, _PATH_DOCBASE);
	strcat(fullpath, file);
	
	if (!stat(fullpath, &st) && (st.st_mode & S_IFMT) == S_IFDIR) {
		if (file[strlen(fullpath) - 1] != '/'){
			strcat(fullpath, "/");
		}
		strcat(fullpath, "index.html");
	}
	
	fin = open(fullpath, O_RDONLY);
	if (fin < 0){
		send_error(fd, 404, "Document (probably) not found");
		return;
	}
	size = lseek(fin, (off_t)0, SEEK_END);
	lseek(fin, (off_t)0, SEEK_SET);
	send_header(fd, get_mime_type(fullpath));
	sprintf(buf,"Content-Length: %ld\r\n\r\n", size);
	write(fd, buf, strlen(buf));
	
	do {
		ret = read(fin, buf, sizeof(buf));
		if (ret > 0)
			ret = write(fd, buf, ret);
	} while (ret == sizeof(buf));
	
	close(fin);
	
}

int main(int argc, char **argv)
{
	int ret, conn_sock;
	struct sockaddr_in localadr;

	if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("httpd");
		return -1;
	}

	/* set local port reuse, allows server to be restarted in less than 10 secs */
	ret = 1;
	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(int)) < 0)
		perror("SO_REUSEADDR");

	/* set small listen buffer to save ktcp memory */
	ret = SO_LISTEN_BUFSIZ;
	if (setsockopt(listen_sock, SOL_SOCKET, SO_RCVBUF, &ret, sizeof(int)) < 0)
		perror("SO_RCVBUF");

	localadr.sin_family = AF_INET;
	localadr.sin_port = htons(DEF_PORT);
	localadr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listen_sock, (struct sockaddr *)&localadr, sizeof(struct sockaddr_in)) < 0) {
		fprintf(stderr, "httpd: bind error (may already be running)\n");
		return 1;
	}
	if (listen(listen_sock, 5) < 0) {
		perror("listen");
		return 1;
	}

	/* become daemon, debug output on 1 and 2*/
	if ((ret = fork()) == -1) {
		perror("httpd");
		return 1;
	}
	if (ret) exit(0);
	ret = open("/dev/null", O_RDWR); /* our log file! */
	dup2(ret, 0);
	dup2(ret, 1);
	dup2(ret, 2);
	if (ret > 2)
		close(ret);
	setsid();

	while (1) {
		conn_sock = accept(listen_sock, NULL, NULL);
		
		if (conn_sock < 0) {
			if (errno == ENOTSOCK)
				exit(1);
			continue;
		}

		if ((ret = fork()) == -1)
			perror("httpd");
		else if (ret == 0) {
			close(listen_sock);
			process_request(conn_sock);
			close(conn_sock);
			exit(0);
		} else {
			close(conn_sock);
			waitpid(ret, NULL, 0);
		}
	}
}
