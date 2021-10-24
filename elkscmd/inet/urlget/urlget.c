/* urlget.c Copyright 2000 by Michael Temari All Rights Reserved
 *
 * 04/05/2000 Michael Temari <Michael@TemWare.Com>
 * 09/29/2001 Ported to ELKS(and linux) (Harry Kalogiroy <harkal@rainbow.cs.unipi.gr>)
 * 10/16/2021 Added ftpput, verbose option and improved error handling <helge@skrivervik.com>
 *
 * Implements HTTP POST & GET, FTP GET,PUT and directory listings, TCP GET (typical via 
 * netcat at the other end.
 * 
 * Assumes hard links to the approproate names - ftpget, urlget, tcpget, tcpput.
 * Optons (http only):
 * 	-h -- include header in output stream
 * 	-d -- discard data (httpget), disable data draining on write errors (ftpget)
 * 	-p -- post instead of get, data to post (ascii/UTF) is appended to the URL (after a '?').
 *	-v -- for ftp, verbose file listing & error reporting, progress meter
 *
 */


#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>


#define _PROTOTYPE(a,b) a b

_PROTOTYPE(char *unesc, (char *s));
_PROTOTYPE(void encode64, (char **pp, char *s));
_PROTOTYPE(int net_connect, (char *host, int port));
_PROTOTYPE(void net_close, (int fd, int errflag));
_PROTOTYPE(char *auth, (char *user, char *pass));
_PROTOTYPE(int skipit, (char *buf, int len, int *skip));
_PROTOTYPE(int httpget, (char *host, int port, char *user, char *pass, char *path, int headers, int discard, int post));
_PROTOTYPE(void ftppasv, (char *reply));
_PROTOTYPE(int ftpreply, (FILE *fpr));
_PROTOTYPE(int ftpcmd, (FILE *fpw, FILE *fpr, char *cmd, char *arg));
_PROTOTYPE(int ftpio, (char *host, int port, char *user, char *pass, char *path, int type, int verbose));
_PROTOTYPE(int tcpget, (char *host, int port, char *user, char *pass, char *path));
_PROTOTYPE(int main, (int argc, char *argv[]));

char ftpphost[15+1];
unsigned int ftppport;
int opt_d = 0;

#define	SCHEME_HTTP	1
#define	SCHEME_FTP	2
#define	SCHEME_TCP	3
#define	SCHEME_NNTP	4

#define TRANS_DEBUG	0	/* for debug dumps */

char buffer[8000];

#if 0
_PROTOTYPE(int strncasecmp, (const char *s1, const char *s2, size_t len));
int
strncasecmp(s1, s2, len)
const char *s1, *s2;
size_t len;
{
        int c1, c2;
        do {
                if (len == 0)
                        return 0;
                len--;
        } while (c1= toupper(*s1++), c2= toupper(*s2++), c1 == c2 && (c1 & c2))
                ;
        if (c1 & c2)
                return c1 < c2 ? -1 : 1;
        return c1 ? 1 : (c2 ? -1 : 0);
}
#endif

char *unesc(char *s) {
   char *p, *p2;
   unsigned char c;

   p = s;
   p2 = s;
   while (*p) {
   	if (*p != '%') {
   		*p2++ = *p++;
   		continue;
   	}
   	p++;
   	if (*p == '%') {
   		*p2++ = *p++;
   		continue;
   	}
   	if (*p >= '0' && *p <= '9') c = *p++ - '0'; else
   	if (*p >= 'a' && *p <= 'f') c = *p++ - 'a' + 10; else
   	if (*p >= 'A' && *p <= 'F') c = *p++ - 'A' + 10; else
   		break;
   	if (*p >= '0' && *p <= '9') c = c << 4 | (*p++ - '0'); else
   	if (*p >= 'a' && *p <= 'f') c = c << 4 | ((*p++ - 'a') + 10); else
   	if (*p >= 'A' && *p <= 'F') c = c << 4 | ((*p++ - 'A') + 10); else
   		break;
   	*p2++ = c;
   }
   *p2 = '\0';
   return(s);
}

void encode64(pp, s)
char **pp;
char *s;
{
char *p;
char c[3];
int i;
int len;
static char e64[64] = {
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9','+','/' };

   p = *pp;
   len = strlen(s);
   for (i=0; i < len; i += 3) {
   	c[0] = *s++;
   	c[1] = *s++;
   	c[2] = *s++;
   	*p++ = e64[  c[0] >> 2];
   	*p++ = e64[((c[0] << 4) & 0x30) | ((c[1] >> 4) & 0x0f)];
   	*p++ = e64[((c[1] << 2) & 0x3c) | ((c[2] >> 6) & 0x03)];
   	*p++ = e64[  c[2]       & 0x3f];
   }
   if (i == len+1)
   	p[-1] = '=';
   else
   if (i == len+2) {
   	p[-1] = '=';
   	p[-2] = '=';
   }
   *p = '\0';
   *pp = p;
   return;
}

char *auth(user, pass)
char *user;
char *pass;
{
static char a[128];
char up[128];
char *p;

   strcpy(a, "BASIC ");
   p = a + 6;
   sprintf(up, "%s:%s", user, pass);
   encode64(&p, up);

   return(a);
}

/*
 * Find the occurence of 2 CR-LF pairs in a row, which indicates the end
 * of the header, start of content.
 */
int skipit(char *buf, int len, int *skip) {

   static int lf = 0;
   static int crlf = 0;
   char *p = buf;

   while (--len >= 0) {
   	if ((crlf == 0 || crlf == 2) && *p == '\r')
   		crlf++;
   	else
   	if ((crlf == 1 || crlf == 3) && *p == '\n')
   		crlf++;
   	else
   		crlf = 0;
   	if (*p == '\n')
   		lf++;
   	else
   		lf = 0;
   	if (crlf == 4 || lf == 2) {
   		*skip = 0;
   		return(len);
   	}
   	p++;
   }

   return(0);
}

int httpget(char *host, int port, char *user, char *pass, char *path, 
	int headers, int discard, int post) {

   int fd, skip, s, s2;
   int len = 0;
   char *a, *qs;

   if (port == 0)
   	port = 80;

   fd = net_connect(host, port);
   if (fd < 0) {
   	fprintf(stderr, "httpget: Could not connect to %s:%d\n", host, port);
   	return(-1);
   }

   if (post) {
   	qs = strrchr(path, '?');
   	if (qs != (char *)NULL) {
   		*qs++ = '\0';
   		len = strlen(qs);
	}
   }

   if (post && len > 0)
	write(fd, "POST ", 5);
   else
	write(fd, "GET ", 4);
   write(fd, path, strlen(path));
   write(fd, " HTTP/1.0\r\n", 11);
   write(fd, "User-Agent: urlget\r\n", 20);
   write(fd, "Connection: Close\r\n", 19);
   if (*user) {
   	write(fd, "Authorization: ", 15);
   	a = auth(user, pass);
   	write(fd, a, strlen(a));
   	write(fd, "\r\n", 2);
   }
   if (post && len > 0) {
   	sprintf(buffer, "Content-Length: %u\r\n", len);
   	write(fd, buffer, strlen(buffer));
   }
   write(fd, "Host: ", 6);
   write(fd, host, strlen(host));
   write(fd, "\r\n", 2);
   write(fd, "\r\n", 2);
   if (post && len > 0)
   	write(fd, qs, len);

   skip = 1;
   while ((s = read(fd, buffer, sizeof(buffer))) > 0) {
   	if (skip) {
   		s2 = skipit(buffer, s, &skip);
   		if (headers)
   			write(1, buffer, s - s2);
   	} else
   		s2 = s;
   	if (s2 && !discard)
		if (write(1, &buffer[s - s2], s2) != s2) {
			perror("write");
			return(-1);
		}
   }

   net_close(fd, 0);	/* send FIN on close */

   return(0);
}

void ftppasv(char *reply) {
char *p;
unsigned char n[6];
int i;

   ftppport = 0;

   p = reply;
   while (*p && *p != '(') p++;
   if (!*p) return;
   p++;
   i = 0;
   while (1) {
   	n[i++] = atoi(p);
   	if (i == 6) break;
   	p = strchr(p, ',');
   	if (p == (char *)NULL) return;
   	p++;
   }
   sprintf(ftpphost, "%d.%d.%d.%d", n[0], n[1], n[2], n[3]);
   ftppport = n[4] * 256 + n[5];
   return;
}

int ftpreply(FILE *fpr) {
static char reply[256];
int s;
char code[4];
int ft;

   do {
   	ft = 1;
   	do {
   		if (fgets(reply, sizeof(reply), fpr) == (char *)NULL)
   			return(-1);
   		if (ft) {
   			ft = 0;
   			strncpy(code, reply, 3);
   			code[3] = '\0';
   		}
   	} while (strncmp(reply, code, 3) || reply[3] == '-');
   	s = atoi(code);
   } while (s < 200 && s != 125 && s != 150);
   if (s == 227) {
     ftppasv(reply);
     return 227;
   }
   return s;
}

int ftpcmd( FILE *fpw, FILE *fpr, char *cmd, char *arg)
{
   int s = 0;
   fprintf(fpw, "%s%s%s\r\n", cmd, *arg ? " " : "", arg);
   fflush(fpw);
   if (strcmp(cmd, "ABOR") != 0) s=ftpreply(fpr); /* Don't wait for reply to ABORT, need to 
						   * purge the connection first
						   * (kludge)	*/
   return s;
}

int ftpio(char *host, int port, char *user, char *pass, char *path, int type, int verbose) {
   int fd;
   int fd2;
   FILE *fpr;
   FILE *fpw;
   int s;
   int s2 = 0;
   char *p;
   char *p2;
   char typec[2], *list;
   int infile = 0;

   if (port == 0)
   	port = 21;

   if (type == '\0')
   	type = 'i';

   if (type == 'S') {	/* ftpput */
	/* FIXME: 
	 * Need to be able to specify destination file/path.
	 */
	if ((infile = open(path, O_RDONLY)) < 0) {
		perror(path);
		return(-1);
	}
	type = 'i';
   }
   fd = net_connect(host, port);
   if (fd < 0) {
   	fprintf(stderr, "ftp: Could not connect to %s:%d\n", host, port);
   	return(-1);
   }
   fpr = fdopen(fd, "r");
   fpw = fdopen(fd, "w");

   s = ftpreply(fpr);
   if (s / 100 != 2) {
	fprintf(stderr, "ftp: Connect error: %d\n", s);
	goto error;
   }
   s = ftpcmd(fpw, fpr, "USER", *user ? user : "ftp");
   if (s / 100 == 3)
   	s = ftpcmd(fpw, fpr, "PASS", *pass ? pass : "urlget@x.com");

   if (s / 100 != 2) {
	fprintf(stderr, "ftp: Authentication failed: %d\n", s);
	goto error;
   }

   p = path;
   if (*p == '/') p++;
   while ((p2 = strchr(p, '/')) != (char *)NULL) {
   	*p2++ = '\0';
   	s = ftpcmd(fpw, fpr, "CWD", unesc(p));
	if ((s/100) != 2) {		/* 250 = success */
		if (infile) {		/* ftpput: Try to create the directory */
   			s = ftpcmd(fpw, fpr, "MKD", unesc(p));
			if ((s/100) != 2) {	/* 257 = success */
				fprintf(stderr, "ftp: Create directory %s failed, error %d\n", p, s);
				goto error;
			} 
			if (verbose) fprintf(stderr, "ftp: Destination directory created: %s\n", p);
   			s = ftpcmd(fpw, fpr, "CWD", unesc(p)); /* assume success */

		} else {
			fprintf(stderr, "ftp: Remote change directory failed: %s -- status %d\n", p, s);
			if (infile) 	/* fatal if we're sending files */
				goto error;
		}
	}
   	p = p2;
   }

   sprintf(typec, "%c", type == 'd' ? 'A' : type);
   s = ftpcmd(fpw, fpr, "TYPE", typec);
   if (s / 100 != 2) {
	fprintf(stderr, "ftp: Type error: %d\n", s);
	goto error;
   }
   if (strlen(p) == 0) type = 'd'; 	/* last char is '/', it's a directory, list files */
   ftppport=0; 				/* to check if retrieved below */

   s = ftpcmd(fpw, fpr, "PASV", "");
 
   if (ftppport==0) {			/* set in ftpcmd */
	fprintf(stderr, "ftp: Error listing directory: %d\n", s);
	goto error; 
   }
   fd2 = net_connect(ftpphost, ftppport);
   if (fd2 < 0) {	
	fprintf(stderr, "ftp: Network connect error.\n");
	goto error;
   }
   if (verbose) 
	list = "LIST"; 
   else 
	list = "NLST";
   if (!infile) { 		/* ftpget */
	s = ftpcmd(fpw, fpr, type == 'd' ? list : "RETR", unesc(p));
   	if ((s/100) != 1) {
		fprintf(stderr, "ftp: Cannot open remote file: %d\n", s);
		goto error;
   	}
	while ((s = read(fd2, buffer, sizeof(buffer))) > 0) {
		if (verbose) fprintf(stderr, ".");
   		s2 = write(1, buffer, s);
		//fprintf(stderr, "wr: %d - %d:", s, s2);
   		if (s2 != s) { s = -1; break; }	/* ERROR */
	}
	if (verbose) fprintf(stderr, ".\n");
	if (s < 0) {
		s = ftpcmd(fpw, fpr, "ABOR", "");
		fprintf(stderr, "ftpget: File write error");
		if (s2 == -1)
			perror("");
		else
			fprintf(stderr,".\n");
		if (!opt_d)
			 while (read(fd2, buffer, sizeof(buffer)) > 0); /* purge */
		s2 = ftpreply(fpr);	/* get the ABORT reply */
		net_close(fd2, 1);	/* send RST on close */
		/* should delete destination file: stdout */
		s = -1;
		goto error;
	}
	s = ftpreply(fpr);
	if (verbose)
		fprintf(stderr,"ftpget/put exit status: %d\n", s);
	s = 0;

   } else { 		/* ftpput */
	// TODO: Add input from stdin
	s = ftpcmd(fpw, fpr, "STOR", unesc(p));
   	if (s / 100 != 1) {
		fprintf(stderr, "ftp: Cannot open destination file: %d\n", s);
		/* 553 is the most common error */
		if (s == 553) fprintf(stderr, "Permission denied.\n");
		goto error;
   	}

	while ((s2 = read(infile, buffer, sizeof(buffer))) > 0) {
		if (verbose) fprintf(stderr, ".");
		s = write(fd2, buffer, s2);
   		if (s2 != s) { s = -1; break; }
	}
	if (verbose) fprintf(stderr, ".\n");
	if (s < 0) {
		s2 = ftpcmd(fpw, fpr, "ABOR", "");
		fprintf(stderr, "ftpput: Network write error.\n");
		s2 = ftpcmd(fpw, fpr, "DELE", unesc(p)); /* delete remote file */
		//DEBUG
		fprintf(stderr, "ftp: Cleaning up - removing %s, status %d\n", p, s);
	} else
		s = 0;
	close(infile);
   }
   net_close(fd2, s);	/* s == 0? FIN: RST */

error:
   (void) ftpcmd(fpw, fpr, "QUIT", "");

   fclose(fpr);
   fclose(fpw);
   net_close(fd, s);	/* s == 0? FIN: RST */

   return(s == 0 ? 0 : -1);
}


int tcpget(char *host, int port, char *user, char *pass, char *path) {

   int fd;
   int s;
   int s2;

   if (port == 0) {
   	fprintf(stderr, "tcpget: No port specified\n");
   	return(-1);
   }

   fd = net_connect(host, port);
   if (fd < 0) {
   	fprintf(stderr, "tcpget: Could not connect to %s:%d\n", host, port);
   	return(-1);
   }
   if (*path == '/')
   	path++;

   write(fd, path, strlen(path));
   write(fd, "\n", 1);
   while ((s = read(fd, buffer, sizeof(buffer))) > 0) {
   	s2 = write(1, buffer, s);
	if (s2 != s) {
	    net_close(fd, 1); /* send RST*/
	    return -1;
	}
   }
   net_close(fd, 0);	/* send FIN*/
   return(0);
}

int main(int argc, char **argv) {

   char *prog, *url, scheme;
   char user[64], pass[64], host[64];
   int port, s;
   int type = 'i';	/* default ftp type */
   char *path, *ps, *p, *at;
   int opt_h = 0, opt_p = 0, opt_v = 0;

   prog = strrchr(*argv, '/');
   if (prog == (char *)NULL)
   	prog = *argv;
   argv++;
   argc--;

   // FIXME: Add getopt ..
   if (argc){
   	if (strcmp(*argv, "-h") == 0) {
   		opt_h = -1;
   		argv++;
   		argc--;
   	}
   	if (strcmp(*argv, "-d") == 0) {
   		opt_d = -1;
   		argv++;
   		argc--;
   	}
   	if (strcmp(*argv, "-p") == 0) {
   		opt_p = -1;
   		argv++;
   		argc--;
   	}
   	if (strcmp(*argv, "-v") == 0) {
   		opt_v = -1;
   		argv++;
   		argc--;
   	}
   }

   if ((strcmp(prog, "ftpget") == 0) || (strcmp(prog, "ftpput") == 0)) {
   	if (argc < 2 || argc > 4) { 
   		fprintf(stderr, "Usage: %s [-v] host[:port] path [user [pass]]\n", prog);
		fprintf(stderr, "Add / to path for directory listing (ftpget), -v for long listing\n");
		fprintf(stderr, "e.g. ftpget 90.147.160.69 /mirrors/\n");
   		return(-1);
   	}
	/* FIXME: Add the ability to specify input file separately for ftpput */

   	strncpy(host, *argv++, sizeof(host));
	if ((p = strchr(host, ':'))) {
		*p++ = '\0';
		port = atoi(p);
	} else
   		port = 21;
   	path = *argv++;
   	if (argc) {
   		strncpy(user, *argv++, sizeof(user));
   		argc++;
   	} else
   		*user = '\0';
   	if (argc) {
   		strncpy(pass, *argv++, sizeof(pass));
   		argc++;
   	} else
   		*pass = '\0';
	if (strcmp(prog, "ftpput") == 0) {
		type = 'S';	/* Always send files as binary, 'S' is the put vs. get flag */
	}
	s = ftpio(host, port, user, pass, path, type, opt_v);
	return(s);
   }
   if (strcmp(prog, "httpget") == 0) {
   	if (argc != 2) {
   		fprintf(stderr, "Usage: %s [-h] [-d] [-p] host[:port] path\n", prog);
   		return(-1);
   	}
   	strncpy(host, *argv++, sizeof(host));
	if ((p = strchr(host, ':'))) {
		*p++ = '\0';
		port = atoi(p);
	} else
   		port = 80;
   	path = *argv++;
	s = httpget(host, port, user, pass, path, opt_h, opt_d, opt_p);
	return(s);
   }

   if (argc != 1) {
   	fprintf(stderr, "Usage: %s [-h] [-p] url\n", prog);
	fprintf(stderr, "e.g. urlget http://216.58.209.67/index.html\n");
   	return(-1);
   }

   url = *argv++;
   argc--;

   if (strncasecmp(url, "http://", 7) == 0) {
   	scheme = SCHEME_HTTP;
   	ps = url + 7;
   } else
   if (strncasecmp(url, "ftp://", 6) == 0) {
   	scheme = SCHEME_FTP;
   	ps = url + 6;
   } else
   if (strncasecmp(url, "tcp://", 6) == 0) {
   	scheme = SCHEME_TCP;
   	ps = url + 6;
   } else {
	fprintf(stderr, "%s: I do not handle this scheme\n", prog);
	return(-1);
   }

   user[0] = '\0';
   pass[0] = '\0';
   host[0] = '\0';
   port = 0;

   p = ps;
   while (*p && *p != '/') p++;
   path = p;
   *path = '\0';

   at = strchr(ps, '@');
   if (at != (char *)NULL) {
   	*at = '\0';
   	p = ps;
   	while (*p && *p != ':') p++;
   	if (*p)
   		*p++ = '\0';
	strcpy(user, ps);
   	strcpy(pass, p);
   	ps = at + 1;
   }

   *path = '/';
   p = ps;
   while (*p && *p != '/' && *p != ':') p++;
   if (*p) {
   	strncpy(host, ps, p - ps);
   	host[p - ps] = '\0';
   }
   if (*p == ':') {
   	p++;
   	ps = p;
   	while (*p && *p != '/')
   		port = port * 10 + (*p++ - '0');
   }
   if (*p == '/')
	path = p;
   else
   	path = "/";
   if (scheme == SCHEME_FTP) {
   	p = path;
   	while (*p && *p != ';') p++;
   	if (*p) {
   		*p++ = '\0';
   		if (strncasecmp(p, "type=", 5) == 0) {
   			p += 5;
   			type = tolower(*p);
   		}
   	}
   }

#if TRANS_DEBUG
   fprintf(stderr, "Host: %s\n", host);
   fprintf(stderr, "Port: %d\n", port);
   fprintf(stderr, "User: %s\n", user);
   fprintf(stderr, "Pass: %s\n", pass);
   fprintf(stderr, "Path: %s\n", path);
   fprintf(stderr, "Type: %c\n", type);
#endif

   switch(scheme) {
   	case SCHEME_HTTP:
		s = httpget(host, port, user, pass, path, opt_h, opt_d, opt_p);
		break;
	case SCHEME_FTP:
		/* no support for ftpput yet */
		s = ftpio(host, port, user, pass, path, type, opt_v);
		break;
	case SCHEME_TCP:
		s = tcpget(host, port, user, pass, path);
		break;
   }

   return(s);
}
