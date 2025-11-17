/*
 * Minimal FTP server for TLVC
 * November 2021 by Helge Skrivervik - helge@skrivervik.com
 *
 * TODO:
 *	- Add ABORT support
 *
 */

#include	<time.h>
#include	<sys/socket.h>
#include	<string.h>
#include	<arpa/inet.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<netinet/in.h>
#include	<netdb.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	<signal.h>
#include 	<dirent.h>

#define		BLOAT

#ifdef BLOAT
#define GLOB	/* Enable local globbing - increases size of the executable significantly */
#endif

#ifdef GLOB
#include	<pwd.h>
#include	<regex.h>
#endif

#define 	CMDBUFSIZ 	512
#define		IOBUFSIZ	1500
#define		TRUE		1
#define		FALSE		0
#define		FTP_PORT	21
#define		MAXARGS 256     /* max names in wildcard expansion */
#define		PASV_PORT	49821U	/* 'random' port for passive connections */
#define		QEMU_PORT	8041U	/* outside port for QEMU */

#ifndef MAXPATHLEN
#define		MAXPATHLEN	256
#endif

#define		UC(b)		(((int) b) & 0xff)

/* Commands */
enum {
	CMD_USER,
	CMD_PASS,
	CMD_ACCT,
	CMD_CWD,
	CMD_CDUP,
	CMD_SMNT,
	CMD_QUIT,
	CMD_REIN,
	CMD_PORT,
	CMD_PASV,
	CMD_TYPE,
	CMD_STRU,
	CMD_MODE,
	CMD_RETR,
	CMD_STOR,
	CMD_STOU,
	CMD_APPE,
	CMD_ALLO,
	CMD_REST,
	CMD_RNFR,
	CMD_RNTO,
	CMD_ABOR,
	CMD_DELE,
	CMD_RMD,
	CMD_MKD,
	CMD_PWD,
	CMD_LIST,
	CMD_NLST,
	CMD_SITE,
	CMD_SYST,
	CMD_STAT,
	CMD_HELP,
	CMD_NOOP,
	CMD_UNKNOWN,
	CMD_EMPTY,
	CMD_CLOSE
};

struct cmd_tab {
	char *cmd;
	int value;
	//char *hlp;
};

struct cmd_tab cmdtab[] = {
	{"LIST", CMD_LIST},
	{"RETR", CMD_RETR},
	{"STOR", CMD_STOR},
	{"ABOR", CMD_ABOR},
	{"SYST", CMD_SYST},
	{"QUIT", CMD_QUIT},
	{"PASV", CMD_PASV},
	{"PORT", CMD_PORT},
	{"TYPE", CMD_TYPE},
	{"DELE", CMD_DELE},
	{"NLST", CMD_NLST},
	{"NOOP", CMD_NOOP},
	{"SITE", CMD_SITE},
	{"PWD", CMD_PWD},
	{"CWD", CMD_CWD},
	{"MKD", CMD_MKD},
	{"RMD", CMD_RMD},
	{"", CMD_UNKNOWN}
};

static int debug = 0;
static int qemu = 0;
static int nofork = 0;
static int timeout = 900;
static int maxtimeout = 7200;
static int controlfd;
static char real_ip[20];

int do_nlist(int, char *);

/* Trim leading and trailing whitespaces */
void trim(char *str) {
	int i;
	int begin = 0;
	int end = strlen(str) - 1;

	if (!*str) return;
	while (isspace((unsigned char) str[begin]))
        	begin++;

	while ((end >= begin) && (isspace((unsigned char) str[end])))
        	end--;

	// Shift all characters back to the start of the string array.
	for (i = begin; i <= end; i++)
        	str[i - begin] = str[i];

	str[i - begin] = '\0'; // Null terminate string.
}

void send_reply1(int code, char *str, char *parm) {
	char cp[70];
	char *sep = ": ";

	if (!*parm)
		sep = "";
	sprintf(cp, "%d %s%s%s\r\n", code, parm, sep, str);
	write(controlfd, cp, strlen(cp));
}

void send_reply(int code, char *str) {
	send_reply1(code, str, "");
} 

void clean(char *s) {	/* chop off CRLF at end of string */
	int l = strlen(s);

	while (l) {
		if (s[l-1] < ' ')
			s[l-1] = '\0';
		else
			break;
		l--;
	}
	trim(s);
	return;
}

void net_close(int fd, int errflag) {
	int ret;
	struct linger l;

	if (errflag) {
		l.l_onoff = 1; /* Force RST on close */
		l.l_linger = 0;
		ret = setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
		if (ret < 0)
			perror("setsockopt FIN");
	}
	close(fd);
}

int get_client_ip_port(char *str, char *client_ip, unsigned int *client_port) {
	char *n[6];
	int p5 = 0, p6;

	// Sample PORT CMD: 'PORT 10,0,2,2,211,37'
	if (strtok(str, " ") == NULL) 
		return -1;
	while (p5 < 6)
		n[p5++] = strtok(NULL, ",");

	sprintf(client_ip, "%s.%s.%s.%s", n[0], n[1], n[2], n[3]);
	p5 = atoi(n[4]);
	p6 = atoi(n[5]);
	*client_port = (256 * p5) + p6;

	return 1;
}

int do_active(char *client_ip, unsigned int client_port, unsigned int server_port) {
	
	struct sockaddr_in cliaddr, tempaddr;
	int fd, sockwait = 0;
	char *ip = client_ip;

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    		perror("socket error");
    		return -1;
	}
#if 1
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
		/* This should not happen */
		printf("Data channel socket error: REUSEADDR, closing channel.\n");
		close(fd);
		return -1;
	}
#endif
	/* port for data connection is server_port-1 - usually 20 */
	bzero(&tempaddr, sizeof(tempaddr));
	tempaddr.sin_family = AF_INET;
	tempaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	tempaddr.sin_port = htons(server_port-1);

	while ((bind(fd, (struct sockaddr*) &tempaddr, sizeof(tempaddr))) < 0) {
		if (sockwait++ > 10 || errno != EADDRINUSE) {
			printf("Bind: Could not connect on port %d\n", server_port-1);
    			perror("bind error");
			return -1;
		} 
		sleep(1);
	}

	/* initiate data connection fd with client ip and client port             */
	bzero(&cliaddr, sizeof(cliaddr));

	if (qemu) ip = real_ip;		/* for QEMU hack, use this IP instead of 
					 * the one sent by the client. */
	//printf("DEBUG: connect to (real) %s/%u\n", ip, client_port);
	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(client_port);
	cliaddr.sin_addr.s_addr = in_aton(ip);

	if (in_connect(fd, (struct sockaddr *) &cliaddr, sizeof(cliaddr), 10) < 0) {
    		perror("connect error");
    		return -1;
	}

	return fd;
}

int get_param(char *input, char *fileptr) {

	char *param;

	param = strtok(input, " ");	/* skip command name */
	param = strtok(NULL, "\r\n");	/* get the rest of the line */
	
	if (param == NULL) {
		*fileptr = '\0';
        	return -1;
	}
    	strcpy(fileptr, param);
    	return 1;
}

int get_command(char *cmdline) {
	struct cmd_tab *c = cmdtab;

	while (*c->cmd != '\0') {
		//printf("<%s> ", c->cmd);
		if (!strncasecmp(cmdline, c->cmd, strlen(c->cmd)))
                        return c->value;
                c++;
        }
        return CMD_UNKNOWN;
}

/* For now - 'fake' login processing - always returns OK */
/* If/when LOGIN is changed to do real user authentication, */
/* move the cmd processing to the main command loop, add LOGOUT */
/* support and use setuid()/seteuid() in the child process. */
/* The plan is to use /etc/passwd authentication and /etc/ftpusers */
/* as supplement. */

int do_login() {
	char buf[200];

	if (read(controlfd, buf, 200) < 0)
		return -1;
	clean(buf);
	if (strncmp(buf, "USER", 4)) 
		return -1;
	send_reply(331, "User OK");

	if (read(controlfd, buf, 200) < 0)
		return -1;
	if (strncmp(buf, "PASS", 4)) 
		return -1;
	chdir("/root");		/* brute force ... */
	send_reply(230, "Password OK");
	return 0; /* always OK */
}

void toolong() {	/* session timeout, close connection */
	char cmd_buf[CMDBUFSIZ];

	sprintf(cmd_buf, "Timeout (%d seconds): closing control connection", timeout);
	send_reply(421, cmd_buf);
}

void reapchildren()
{
	while (wait4(-1, NULL, WNOHANG, NULL) > 0)
		;
	signal(SIGCHLD, reapchildren);
}

int checknum(char *s) {
	while (*s != '\0')
		if (!isdigit(*s++)) return 0;
	return 1;
}

/*
 * Two quite different functions: LIST (DIR command) which returns a ls -l output
 * and NLST (nlist) which returns filenames with paths if approriate. In the first case
 * the shell will do wildcard processing, in the latter, we have to do it (if GLOB is defined).
 * In both cases we should check for some existence, so as to not return an empty list (no
 * message) which will look like an error 'wtf happened?') to the user at the other end.
 */
int do_list(int datafd, char *input) {
	char cmd_buf[CMDBUFSIZ], iobuf[IOBUFSIZ];
   	FILE *in;
	int len, nlst = 0, status = 0;

	//bzero(cmd_buf, sizeof(cmd_buf));
	bzero(iobuf, sizeof(iobuf));

	if (strncmp(input, "NLST", 4) == 0)
		nlst++;
	if (get_param(input, iobuf) > 0)
		trim(iobuf);
	else
		*iobuf = '.';	

	if (nlst) {
		if (debug) printf("NLST <%s>\n", iobuf);
		status = do_nlist(datafd, iobuf);
	} else {
		/* LIST - pipe output from ls-command back to the client */
		/*        let /bin/sh do the error reporting */

		sprintf(cmd_buf, "/bin/ls -l %s 2>&1", iobuf);

#ifdef LET_LS_DO_THIS	/* wildcards are OK, so we either let the shell do the processing */
			/* or we do full wildcard processing here */
		if (access(iobuf, F_OK)) {
			send_reply1(550, "No such file or directory", iobuf);
    			return -1;
    		} 
#endif
		if (!(in = popen(cmd_buf, "r"))) {
			send_reply(451, "Requested action aborted. Local error in processing");
        		return -1;
		}
		//printf("LIST: command is '%s'\n", cmd_buf);
		send_reply(150,"Opening ASCII mode data connection for /bin/ls");

		bzero(iobuf, sizeof(iobuf));
		while (fgets(iobuf, IOBUFSIZ-2, in) != NULL) {
			len = strlen(iobuf);
			iobuf[len-1] = '\r';    /* Change to FTP ASCII-style line endings */
			iobuf[len++] = '\n';
			if (write(datafd, iobuf, len) != len) {
				perror("LIST: Data socket write error");
				status = -1;
				break;
			}
			bzero(iobuf, sizeof(iobuf));
		}
		pclose(in);
	}
	close(datafd);

	return status;
}

/* NLST processing, send a list of names, one per line. If the given name is a
 * directory, return the content of that directory, path included
 */
int do_nlist(int dfd, char *iobuf)
{
	int len = 0, i = 0, count = 0;
#ifdef GLOB
	char *myargv[MAXARGS];

	count = expandwildcards(iobuf, MAXARGS, myargv);
	if (!count) {			// may be a plain file, no expansion
#endif
		clean(iobuf);	/* expandwildcards adds CRLF regardless */
		if (access(iobuf, F_OK) == 0) {
			strcat(iobuf, "\r\n");
			len = strlen(iobuf);
		} else {
                        send_reply1(550, "No such file or directory", iobuf);
			return -1;
		}
	
#ifdef GLOB
	} else
		*iobuf = '\0';

	send_reply(125, "List started OK");
	if (debug > 1) printf("ftpd: nlst %s got %d\n", iobuf, count);
	len = 0;

	while (i < count) {			/* list the wildcard matches */
		if (len + strlen(myargv[i] + 3) < IOBUFSIZ) {
			strcat(iobuf, myargv[i]);
			strcat(iobuf, "\r\n");
			len += strlen(myargv[i])+2;
			i++;
			//printf("LIST: %s (%d)\n", myargv[i], len);
		} else {
			write(dfd, iobuf, len);
			len = 0;
			bzero(iobuf, IOBUFSIZ);
		}
	}
	if (len)
		write(dfd, iobuf, len);
#else
	send_reply(125, "List started OK");
#endif

	if (!count) {	/* if this is a directory, list it */
		struct stat sb;
		DIR *dir;
		struct dirent *dp;
		char thisdir[MAXNAMLEN+2];
		int slen = 0;

		clean(iobuf);
		bzero(thisdir, MAXNAMLEN+2);
		stat(iobuf, &sb);
		if (S_ISDIR(sb.st_mode)) {
			dir = opendir(iobuf);
			if (*(unsigned *)iobuf != (unsigned)'.') { /* Don't prepend current dir */
			    strcpy(thisdir, iobuf);
			    slen = strlen(thisdir);
			    if (*(unsigned *)iobuf != (unsigned)'/') 	  /* Avoid double slash */
			    	thisdir[slen++] = '/';
			} else *thisdir = '\0';
			*iobuf = '\0';
			len = 0;
			while ((dp = readdir(dir)) != NULL) {
			    if (*dp->d_name != '.') {
				if (len + slen + dp->d_namlen + 3 < IOBUFSIZ) {
				    strcat(iobuf, thisdir);
				    strcat(iobuf, dp->d_name);
				    strcat(iobuf, "\r\n");
			    	    //printf("name: %s(%d)\n", &iobuf[len], dp->d_namlen);
				    len += slen + dp->d_namlen + 2;
				} else {
				    write(dfd, iobuf, len);
				    len = 0;
				    bzero(iobuf, IOBUFSIZ);
				}
			    }
			}
			if (len) 
				write(dfd, iobuf, strlen(iobuf));
			closedir(dir);

		} else {	/* the provided name was not a directory, not a wildcard,
				 * just print it */
			if (len) {
				strcat(iobuf, "\r\n");
				write(dfd, iobuf, strlen(iobuf));
				if (debug > 1)
				    printf("NLST <%s>, count %d\n", iobuf, count);
			}
		}
	}
	return 0;
}

/* Passive mode: Server listens for incoming data connection */
int do_pasv(int *datafd) {
	int fd;
	unsigned int i = 1, port = 0;
	struct sockaddr_in pasv;
	char *p, *a;
	char str[100], *pasv_err = "425 Can't open passive connection.\r\n";

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    		write(controlfd, pasv_err, strlen(pasv_err));
		perror("PASV: Socket open failed");
		return -1;
	}

	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0)
		perror("SO_REUSEADDR");
	i = SO_LISTEN_BUFSIZ;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0)
                perror("SO_RCVBUF");

	pasv.sin_family = AF_INET;
	pasv.sin_addr.s_addr = htonl(INADDR_ANY);
	if (qemu) port = PASV_PORT;	// Force predictable port #
	pasv.sin_port = htons(port);
	i = 0;
	while (bind(fd, (struct sockaddr *)&pasv, sizeof(pasv)) < 0) {
		if (debug) printf("PASV bind failed: host %s port %u\n", in_ntoa(pasv.sin_addr.s_addr), ntohs(pasv.sin_port));
		perror("PASV bind failure");
		if (i++ > 10 || errno != EADDRINUSE) {
			if (debug) printf("Bind: Could not connect on port %u\n", port);
    			perror("bind error");
    			write(controlfd, pasv_err, strlen(pasv_err));
			close(fd);
			return -1;
		} 
		port++;
		if (qemu) 
			if (port >= (PASV_PORT + 9)) port = PASV_PORT;
    		pasv.sin_port = htons(port);
	}
	i = sizeof(pasv);
	if (getsockname(fd, (struct sockaddr *) &pasv, (unsigned int *)&i) < 0) {
		perror("getsockname");
		//return -1;
	}
	if (debug) printf("getsockname: adr %s, port %u\n", in_ntoa(pasv.sin_addr.s_addr), ntohs(pasv.sin_port));
	if (listen(fd, 1) < 0) {
    		write(controlfd, pasv_err, strlen(pasv_err));
		if (debug) perror("PASV");
		close(fd);
		return -1;
	}

	a = (char *) &pasv.sin_addr;
	p = (char *) &pasv.sin_port;
	sprintf(str, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
		UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));

	if (qemu) {
		unsigned int qport = QEMU_PORT + port - PASV_PORT;
		sprintf(str, "227 Entering Passive Mode (127,0,0,1,%u,%u)\r\n",
			htons(qport)&0xff, htons(qport)>>8);
	}
    	write(controlfd, str, strlen(str));
	if (debug) printf("%s", str);
	i = sizeof(pasv);
	//FIXME: The accept() will block forever - this will happen in QEMU if an external client
	// requests a passive mode connection.
	if ((*datafd = accept(fd, (struct sockaddr *)&pasv, (unsigned int *)&i)) < 0 ) {
		perror("accept error");	
		close(fd);
		return -1;
	}
	close(fd);
	if (debug) 
		printf("Accepted connection from %s/%u on fd %d\n", in_ntoa(pasv.sin_addr.s_addr), ntohs(pasv.sin_port), *datafd);

	return 0;
}

int do_retr(int datafd, char *input){
	char cmd_buf[CMDBUFSIZ], iobuf[IOBUFSIZ];
	int fd, len;
	struct stat fst;

	//bzero(cmd_buf, sizeof(cmd_buf));
	bzero(iobuf, sizeof(iobuf));

	if (get_param(input, cmd_buf) > 0) {
		trim(cmd_buf);
		if (debug > 1) printf("RETR: <%s> fd %d\n", cmd_buf, datafd);

		stat(cmd_buf, &fst);		/* should handle symlinks if the target is a regular file */
		if (S_ISREG(fst.st_mode) && (fd = open(cmd_buf, O_RDONLY)) > 0 ) {
			sprintf(iobuf, "150 Opening BINARY data connection for %s (%ld bytes).\r\n", cmd_buf, fst.st_size);
			write(controlfd, iobuf, strlen(iobuf));
			while ((len = read(fd, iobuf, sizeof(iobuf))) > 0) 
			    if (write(datafd, iobuf, len) != len) {
				//printf("RETR error fd %d len %d\n", datafd, len);
				perror("Data write error"); 
				break;
			    }
			close(fd);
		} else {
			char *msg;
			if (access(cmd_buf, F_OK))
			    msg = "No such file or directory";
			else
			    msg = "Not a plain file";
			send_reply1(550, msg, cmd_buf);
    			return -1;
		}
	} else { 
		if (debug) printf("RETR: Command needs parameter, no action.\n");
		return -1;
	}
	return 1;
}

int do_stor(int datafd, char *input) {
	char cmd_buf[CMDBUFSIZ], iobuf[IOBUFSIZ];
	int fp;
	int n = 0, len;

	//bzero(cmd_buf, sizeof(cmd_buf));
	bzero(iobuf, sizeof(iobuf));


	if (get_param(input, cmd_buf) < 0) {
		//if (debug) printf("No file specified.\n");
		send_reply(450, "Requested action not taken - no file");
		return -1;
	}

	/* FIXME - should query protection mode for the source file and use that */
	//trim(cmd_buf);
	if ((fp = open(cmd_buf, O_CREAT|O_RDWR|O_TRUNC, 0644)) < 1) {
		perror("File create failure");
		send_reply(552, "File create error, transfer aborted");
		return -1;
	}

	sprintf(iobuf, "Opening BINARY data connection for '%s'", cmd_buf);
	send_reply(150, iobuf);
	while ((n = read(datafd, iobuf, sizeof(iobuf))) > 0) {
		if ((len = write(fp, iobuf, n)) != n) {
			if (len < 0 )
				perror("File write error");
			else
				printf("File write error: %s\n", cmd_buf);
			close(fp);
			send_reply(552, "Storage space exceeded, transfer aborted");
			return -2;
		}
	}

	close(fp);
	return 1;
}

void usage() {
	printf("Usage: ftpd [-d] [-D] [-q] [<listen-port>]\n");
	exit(1);
}


int main(int argc, char **argv) {
	int listenfd, ret;
	unsigned int myport = FTP_PORT;
	struct sockaddr_in servaddr, myaddr;
	char *cp;


	while (--argc) {
		argv++;
		if (*argv[0] == '-') {
			if (argv[0][1] == 'd')
				debug++;
			else if (argv[0][1] == 'D') 
				nofork++;
			else if (argv[0][1] == 'q') {
				qemu++;
			} else
				usage();
		} else {
			myport = atoi(argv[0]);
			break;	/* ignore rest of command line if any */
		}
	}
	if ((cp = getenv("QEMU")) != NULL) 
		qemu = atoi(cp);
	if (qemu && debug) {
		printf("QEMU mode\n");
	}
		
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

	/* set local port reuse, allows server to be restarted in less than 10 secs */
	ret = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret)) < 0)
		perror("SO_REUSEADDR");

	/* set small listen buffer to save ktcp memory */
	ret = SO_LISTEN_BUFSIZ;
	if (setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &ret, sizeof(int)) < 0)
		perror("SO_RCVBUF");

	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port	= htons(myport);
	
	if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0 ) {
		perror("Cannot bind socket");
		exit(2);
	}

	if (listen(listenfd, 1) < 0 ) {
		perror("Error in listen");
		exit(3);
	}
	ret = sizeof(myaddr);	/* save my own address for later use */
	if (getsockname(listenfd, (struct sockaddr *) &myaddr, (unsigned int *)&ret) < 0) {
		perror("getsockname");
		//return -1;
	}
	//if (debug) printf("ftpd running - debug level %d.\n", debug);
	if (!nofork) {
		/* become daemon, debug output on 1 and 2*/
		if ((ret = fork()) == -1) {
			fprintf(stderr, "ftpd: Can't fork to become daemon\n");
			exit(1);
		}
		if (ret) exit(0);
		ret = open("/dev/console", O_RDWR);
		close(STDIN_FILENO);
		dup2(ret, STDOUT_FILENO);
		dup2(ret, STDERR_FILENO);
		if (ret > STDERR_FILENO)
			close(ret);
		setsid();
	} else
		printf("Debug: Not disconnecting from terminal.\n");

	struct sockaddr_in client;
	ret = sizeof(client);
	while (1) {
		int kcount = 0;				/* avoid looping in case of errors */

		if ((controlfd = accept(listenfd, (struct sockaddr *)&client, (unsigned int *)&ret)) < 0) {
			/* socket code bug, retruns error code 512 on interrupt */
			if ((errno == 512 || errno == EINTR) && !kcount++) continue;
			perror("Accept error");
			break;
		}

		waitpid(-1, NULL, WNOHANG);		/* Insurance, handled in reapchildren() */
		if (debug) printf("Accepted connection from %s:%u.\n",
			in_ntoa(client.sin_addr.s_addr), ntohs(client.sin_port));

		if ((ret = fork()) == -1)       /* handle new accept*/
			fprintf(stderr, "ftpd: No processes\n");
		else if (ret != 0) {
			close(controlfd);
			signal(SIGCHLD, reapchildren);
		} else {					/* child process */

			int datafd = -1, code, quit = FALSE;
			unsigned int client_port = 0;
			//char type = 'I';		/* treat everything as binary */
			char client_ip[50], command[CMDBUFSIZ], namebuf[MAXPATHLEN];
			char *str;
			char *complete = "226 Transfer Complete.\r\n";

			close(listenfd);
			signal(SIGCHLD, SIG_DFL);

			strncpy(real_ip, in_ntoa(client.sin_addr.s_addr), 20); // Save for QEMU hack
			/* Turn off qemu mode if we're talking to ourselves (loopback) */
			if (qemu && (myaddr.sin_addr.s_addr == client.sin_addr.s_addr)) {
				if (debug) printf("Loopback detected, disabling qemu mode.\n");
				qemu = 0; 
			}
			if (debug)
				printf("local: %s, remote: %s, QEMU: %d\n", 
					in_ntoa(myaddr.sin_addr.s_addr), real_ip, qemu);

			send_reply(220, "TLVC minimal FTP server speaking");

			/* standard housekeeping */
			if (do_login(controlfd) < 0) {
				printf("Login failed, closing session.\n");
				close(controlfd);
				break;
			}

			/* Main command loop */
			while(1) {
				signal(SIGALRM, toolong);
				alarm(timeout);
				bzero(command, (int)sizeof(command));
				if (read(controlfd, command, sizeof(command)) <= 0) 
					break;
				alarm(0);
				clean(command);
    				code = get_command(command);
    				//if (debug) printf("cmd: '%s' %d\n", command, code);

				switch (code) {

				case CMD_PORT:
					if (datafd >= 0) { /* connection already open, close it! */
						close(datafd);
						datafd = -1;
					}
    					if (get_client_ip_port(command, client_ip, &client_port) < 0) {
						printf("PORT cmd error.\n");
						break;
					}
    					if ((datafd = do_active(client_ip, client_port, myport)) < 0) {
						if (debug) printf("PORT command failed.\n");
						datafd = -1;
						send_reply(425, "Can't open data connection");
					} else 
						send_reply(200, "PORT command successful");
					break;

				case CMD_PASV: /* Enter Passive mode */
					if (datafd >= 0) { /* connection already open, close it! */
						close(datafd);
						datafd = -1;
					}
					if (do_pasv(&datafd) < 0) {
						send_reply(502, "PASV: Cannot open server socket");
						close(datafd);
						datafd = -1;
					}
					break;

				case CMD_NLST:
    				case CMD_LIST:	/* List files */
					if (datafd >= 0) {
    						if (!do_list(datafd, command))
		    				    write(controlfd, complete, strlen(complete));
					} else {
						send_reply(503, "Bad sequence of commands");
						close(datafd);
					}
					datafd = -1;
					break;

    				case CMD_RETR: /* Retrieve files */
					if (datafd < 0) { /* no data connection, don't even try ... */
						send_reply(426, "Connection closed, transfer aborted");
						break;
					}
    					if (do_retr(datafd, command) > 0 ) 
		    				write(controlfd, complete, strlen(complete));
					/* if there was an error, the error reply has already been sent, 
					 * this does not make sense */
					usleep(1000);	/* Experimental */
					close(datafd);
					datafd = -1;
					break;

    				case CMD_STOR: /* Store files */
					if (datafd < 0) { /* no data connection, don't even try ... */
						send_reply(426, "Connection closed, transfer aborted");
						break;
					}
    					if ((code = do_stor(datafd, command)) > 0) {
		    				write(controlfd, complete, strlen(complete));
						close(datafd);
					} else {
						if (code == -2)
							/* must force close */
							net_close(datafd, 1); /* Error: Need RST to stop data flow */
						else close(datafd);
					}
					datafd = -1;
					break;

				case CMD_ABOR: /* ABORT FIXME */
					if (debug) printf("Got ABORT\n");
					break;

				case CMD_SYST: /* Identify ourselves */
					send_reply(215, "UNIX Type: L8 (Linux)");
					break;

				case CMD_TYPE: 	/* ASCII or binary, ignored for now */
						/* DIR is always ascii, files are always binary */
					//str = strtok(command, " ");
					//str = strtok(NULL, " ");
					//type = *str;
					send_reply(200, "Command OK");
					break;
#ifdef BLOAT
				case CMD_MKD:
					//bzero(namebuf, sizeof(namebuf));
					if (get_param(command, namebuf) < 0) {
						send_reply(501, "Syntax error - MKDIR needs parameter");
					} else {
						trim(namebuf);
						if (mkdir(namebuf, 0755) < 0) {
							if (debug) printf("Cannot create directory %s\n", namebuf);
							send_reply(550, "No action - DIR not created");
						} else {
							send_reply(250, "MKDIR successful");
						}
					}
					break;

				case CMD_RMD:
					//bzero(namebuf, sizeof(namebuf));
					if (get_param(command, namebuf) < 0) {
						send_reply(501, "Syntax error - RMD needs parameter");
					} else {
						trim(namebuf);
						if (rmdir(namebuf) < 0) {
							if (debug) printf("Cannot delete %s\n", namebuf);
							send_reply(550, "No action - file not found");
						} else
							send_reply(250, "RMDIR successful");
					}
					break;

				case CMD_DELE:
					//bzero(namebuf, sizeof(namebuf));
					if (get_param(command, namebuf) < 0) {
						send_reply(501, "Syntax error - DELE needs parameter");
					} else {
						trim(namebuf);
						if (unlink(namebuf) < 0) {
							if (debug) printf("Cannot delete %s\n", namebuf);
							send_reply(550, "No action - file not found");
						} else {
							send_reply(250, "Delete successful");
						}
					}
					break;
#endif
				case CMD_NOOP:
					sprintf(command, "200 Howdy.\r\n");
		    			write(controlfd, command, strlen(command));
					break;

				case CMD_PWD:
					str = getcwd(namebuf, MAXPATHLEN); 
					sprintf(command, "257 \"%s\" is current directory.\r\n", str);
		    			write(controlfd, command, strlen(command));
					break;

				case CMD_CWD:
					/* FIXME: if no arg, change back to home dir */
					//bzero(namebuf, sizeof(namebuf));
					if (get_param(command, namebuf) < 0) {
						send_reply(501, "Syntax error - CWD needs parameter");
					} else {
						trim(namebuf);
						if (chdir(namebuf) < 0) {
							if (debug) printf("Cannot chdir to %s\n", namebuf);
							send_reply(550, "No action - directory not found");
						} else 
							send_reply(250, "CWD successful");
					}
					break;
#ifdef BLOAT
				case CMD_SITE:	/* allow client to set or query server idle timeout */
					//bzero(namebuf, sizeof(namebuf));
					if (get_param(command, namebuf) < 0) {
						send_reply(501, "Syntax error - SITE needs subcommand");
						break;
					}
					if (!strncasecmp(namebuf, "DEBU", 4)) {
						debug++;
						send_reply(200, "Debug level increased");
						break;
					}
					if (!strncasecmp(namebuf, "QEMU", 4)) {
						qemu++;
						qemu &= 1;
						if (qemu) 
							send_reply(200, "QEMU mode now active");
						else 
							send_reply(200, "QEMU mode deactivated");
						break;
					}
					if (strncasecmp(namebuf, "IDLE", 4)) {
						send_reply(501, "Error - unsupported SITE subcommand");
						break;
					}
					if (strlen(namebuf) < 6) {
						sprintf(command, "Current idle time limit is %d seconds, max %d",
							timeout, maxtimeout);
						send_reply(200, command);
					} else {
						int ii = 0;
						trim(&namebuf[5]);
						if (checknum(&namebuf[5])) 
							ii = atoi(&namebuf[5]);
						if (ii < 30 || ii > maxtimeout) {
							sprintf(command, "Max idle time must be between 30 and %d seconds", 
								maxtimeout);
							send_reply(501, command);
							break;
						}
						timeout = ii;
						sprintf(command, "Idle time set to %d seconds", timeout);
						send_reply(200, command);
					}
					break;
#endif
				case CMD_CLOSE:	/* Since login is outside the main loop, CLOSE means QUIT */
				case CMD_QUIT:
					quit = TRUE;
					send_reply(221, "Goodbye");
					close(datafd);
					break;

				case CMD_UNKNOWN:
				default:
					send_reply(503, "Unknown command");
					break;
				}
				if (quit == TRUE) break;

			}
			alarm(0);
    			if (debug) printf("Child process exiting...\n");
    			close(controlfd);
    			_exit(0);
							/* End child process */
		}
	}
}


