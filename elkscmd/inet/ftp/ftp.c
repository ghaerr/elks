/*
 *
 * Minimal FTP client for ELKS 
 * by Helge Skrivervik - helge@skrivervik.com - December 2021
 *
 *	TODO:
 *	- split open/login into its own function, move them to the main command loop
 *	  (remove the requirement to have user name and pw on the command line)
 *	- cmd line server address argument must lookup in /etc/hosts
 *	- add support for multi-argument commands (e.g. <put sourcefile destfile> ...)
 *	- add missing commands (rmdir, mls, nlist, ...), 
 *	- Add port # to the open command
 *	- ... create common function for commands that are alomost alike
 *	- clean up messags and debug levels
 *	- fix trucation of file names in FAT, minix (warnings, name collisions)
 *	- lcd with no params should print current local directory
 *	- evaluate whether using select() is useful or just a complication
 *	... add ABORT support in PUT
 *	- handle server timeout (no activity): Unsolicited server input (code 421)
 *	- add '15632 bytes sent in 0.00 secs (27.5561 MB/s)' type message after transfer
 *	- more testing of error conditions & error messages
 */

#define BLOATED		/* fully featured if defined */
#ifdef  BLOATED
#define GLOB		/* include wildcard processing */
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#ifdef GLOB
#include <pwd.h>
#include <regex.h>
#endif


#define	IOBUFLEN 1500
#define BUF_SIZE 512
#define ADDRBUF	50
#define NLSTBUF 1500
#define MAXARGS	256	/* max names in wildcard expansion */

#define	TRUE	1
#define	FALSE	0
#define BIN	'I'
#define ASCII	'A'
#define PASV	0
#define PORT	1

#define UC(b) (((int) b) & 0xff)
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

extern int errno;
extern char *getpass(char *prompt);
static void settype(int controlfd, char t);
static int do_passive(int cmdfd);
static int do_active(int cmdfd);

static FILE *fcmd;

enum {	// commands in disconnected mode
	CMD_OPEN,	// Must be first!!
	CMD_USER,
	CMD_NOCONNECT,	//flag
	// commands in connected mode
	CMD_CWD,
	CMD_GET,
	CMD_PUT,
	CMD_RMD,
	CMD_MKD,
	CMD_PWD,
	CMD_DIR,
	CMD_MPUT,
	CMD_DELE,
	CMD_MGET,
	CMD_CLOSE,
	CMD_CONNECT,	// flag
	// commands in any mode
	CMD_QUIT,
	CMD_PORT,
	CMD_PASV,
	CMD_TYPE,
	CMD_BIN,
	CMD_NULL,
	CMD_SYST,
	CMD_STAT,
	CMD_HELP,
	CMD_ASCII,
	CMD_PROMPT,
	CMD_LCWD,
	CMD_DEBUG,
	CMD_STATUS,
	CMD_VERB,
	CMD_GLOB,
	CMD_SHELL
};

struct cmd_tab {
	char *cmd;
	int value;
	char *hlp;
};

struct cmd_tab cmdtab[] = {
	{"directory", CMD_DIR, "List remote directory"},
	{"ls", CMD_DIR, "List remote directory"},
	{"get", CMD_GET, "Fetch remote file"},
	{"mget", CMD_MGET, "Fetch multiple remote files"},
	{"put", CMD_PUT, "Send file to remote"},
	{"mput", CMD_MPUT, "Send multiple files to remote"},
	{"cd", CMD_CWD, "Change (remote) working directory"},
	{"pwd", CMD_PWD, "Show (remote) working directory"},
	{"bin", CMD_BIN, "Set file transfer type to BINARY"},
	{"ascii", CMD_ASCII, "Set file transer type to ASCII"},
	{"quit", CMD_QUIT, "Quit program"},
	{"type", CMD_TYPE, "Show file transfer type."},
	{"prompt", CMD_PROMPT, "Toggle interactive multifile transfers"},
	{"debug", CMD_DEBUG, "Set debug level 1-4 or off."},
	{"bye", CMD_QUIT, "Leave program"},
#ifdef BLOATED
	{"!", CMD_SHELL, "Invoke shell to execute local command."},
	{"?", CMD_HELP, "Alias for help."},
	{"help", CMD_HELP, "Print this."},
	{"close", CMD_CLOSE, "Close connection."},
	{"open", CMD_OPEN, "Open new connection, log in."},
	{"mkdir", CMD_MKD, "Create remote directory."},
	{"delete", CMD_DELE, "Delete remote file."},
	{"verbose", CMD_VERB, "Toggle debug off or 1, default 1."},
	{"status", CMD_STATUS, "Show status, modes etc."},
	{"lcd", CMD_LCWD, "Change local working directory"},
	{"glob", CMD_GLOB, "Toggle globbing"},
	{"passive", CMD_PASV, "Toggle passive/active mode"},
#endif
	//{"user", CMD_USER, "Open new connection, log in."},
	{"", CMD_NULL, ""}
};
static int debug = 0;
static char type = ASCII, atype = ASCII;
static int prompt = TRUE, glob = TRUE;

/* Trim leading and trailing whitespaces */
void trim(char *str) {

	int i, begin = 0;
	int end = strlen(str) - 1;

	while (isspace((unsigned char) str[begin])) begin++;

	while ((end >= begin) && isspace((unsigned char) str[end])) end--;

	/* Shift all characters to the start of the string. */
	for (i = begin; i <= end; i++)
			str[i - begin] = str[i];

	str[i - begin] = '\0'; // Null terminate string.
}

#ifdef BLOATED
void help() {
	struct cmd_tab *c = cmdtab;	

	while (*c->cmd != '\0') {
		printf("%s:\t%s\n", c->cmd, c->hlp);
		c++;
	}
	return;
}
#endif

/* Interactive mget/mput */

int ask(char *source, char *name) {
	char buf[20];

	if (prompt == FALSE) return 0;
	printf("%s %s (y/n/q): ", source, name);
	while (1) {
		gets(buf);
		if (tolower(*buf) == 'q') return -1;
		if (tolower(*buf) == 'y' || *buf == '\0') return 0;
		if (tolower(*buf) == 'n') return 1;
	}
	/*NOTREACHED*/
	//return 1;
}
		
/* Read (and echo) reply from server */

int get_reply(int fd, char *buf, int size, int dbg) {

	if (fgets(buf, size, fcmd) == NULL)
		return -1;
	/* dbg - at which debug level this command should be echoed */
	if (debug >= dbg) printf("%s", buf);
	return 1;
}

/* Send command to server, echo if required */

int send_cmd(int cmdfd, char *cmd) {
	int s;

	s = write(cmdfd, cmd, strlen(cmd));
	if (debug > 1) printf("---> %s", cmd);
	return s;
}
	
/* Read and process user commands */

int get_cmd(char *buffer, int len) {
	struct cmd_tab *c;
	int cmdlen;

	do {
		printf("ftp> ");
		if (fgets(buffer, len, stdin) == NULL)
			return -1;
		if (*buffer == '\4') return -1; // ^D
	} while (strlen(buffer) < 2);

	buffer[strlen(buffer)-1] = '\0';	/* remove cf/lf */

	if (strchr(buffer, ' '))
		cmdlen = (int)(strchr(buffer, ' ') - buffer);
	else
		cmdlen = strlen(buffer);

	c = cmdtab;
	while (*c->cmd != '\0') {
		if (*buffer == '!') return CMD_SHELL;
		if (*buffer == '?') return CMD_HELP;
		//FIXME: Need to check command uniqueness @ 3 chars
		if (!strncmp(buffer, c->cmd, MAX(cmdlen, MIN(strlen(c->cmd), 3))))
			return c->value;
		c++;
	}
	return CMD_NULL;
}

/* Create address/port# string for PORT command */

int get_port_string(char *str, char *ip, int p0, int p1) {
	int i = 0;
	char itmp[ADDRBUF];
	strcpy(itmp, ip);

	for (i = 0; i < strlen(ip); i++) {
		if (itmp[i] == '.') itmp[i] = ',';
	}
	sprintf(str, "PORT %s,%d,%d\r\n", itmp, p0, p1);
	return 1;
}


int get_client_ip_port(char *str, char *client_ip, unsigned int *client_port) {
	char *n1, *n2, *n3, *n4, *n5, *n6;
	int p5, p6;

	if (strtok(str, "(") == NULL)
		return -1;
	n1 = strtok(NULL, ",");
	n2 = strtok(NULL, ",");
	n3 = strtok(NULL, ",");
	n4 = strtok(NULL, ",");
	n5 = strtok(NULL, ",");
	n6 = strtok(NULL, ")");

	sprintf(client_ip, "%s.%s.%s.%s", n1, n2, n3, n4);

	p5 = atoi(n5);
	p6 = atoi(n6);
	*client_port = (256 * p5) + p6;

	return 1;
}

int get_ip_port(int fd, char *ip, int *port) {
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	getsockname(fd, (struct sockaddr*) &addr, &len);
	sprintf(ip, in_ntoa(addr.sin_addr.s_addr));
	*port = (unsigned int)ntohs(addr.sin_port);
	return 1;
}

//FIXME: Add multi-parameter support, return in a **argv type array
int get_parm(char *input, char *fileptr) {
	char cpy[MAXPATHLEN];
	char *parm;

	strcpy(cpy, input);
	trim(cpy);
	//printf("get_parm: %s ", cpy);
	parm = strtok(cpy, " ");
	parm = strtok(NULL, " \r\n\t");

	if (parm == NULL){
		fileptr = "\0";
		return -1;
	} else {
		strcpy(fileptr, parm);
		return 1;
	}
}


int dataconn(int fd) {		/* wait for the actual data connection in active mode */
	struct sockaddr_in myaddr;
	int datafd, i = sizeof(myaddr);

	if ((datafd = accept(fd, (struct sockaddr *)&myaddr, (unsigned int *)&i)) < 0) {
		perror("accept error");
		close(fd);
		return -1;
	}
	if (debug > 1) 
		printf("Accepted connection from %s @ %u\n", in_ntoa(myaddr.sin_addr.s_addr), ntohs(myaddr.sin_port));
	close(fd);
	return datafd;
}

/* Get a list of filenames from remote, for use in MGET */
/* FIXME: The length of the file list is too limited */

int do_nlst(int controlfd, char *buf, int len, char *dir, int mode) {
	int n, i, datafd;
	char lbuf[BUF_SIZE];

	if (atype != ASCII) settype(controlfd, ASCII);
#ifdef BLOATED
	if (mode != PASV)
		datafd = do_active(controlfd);
	else
#endif
		datafd = do_passive(controlfd);
	if (datafd <= 0) {
		printf("NLST: Failed to open data connection.\n");
		return -1;
	}

	sprintf(lbuf, "NLST %s\r\n", dir);
	send_cmd(controlfd, lbuf);

	if (mode == PORT) {	/* active mode, wait for connection */
		if ((n = dataconn(datafd)) < 0) {
			printf("Active mode connect failed.\n");
			return -1;
		}
		datafd = n;
	}
	get_reply(controlfd, lbuf, sizeof(lbuf), 1);
	if (*lbuf != '1') {		/* 125 List started OK */
		printf("Get filelist failed: %s", buf);
		return -1;
	}
	n = 0;
	bzero(buf, len);
	while (n < len) {
		if ((i = read(datafd, &buf[n], len - n)) <= 0) break;
		n =+ i;
	}
	if (n >= len) {
		printf("Warning: File list too long, truncated.\n");
		n = len - 1;
		while (buf[n] != '\n') n--;
	}
	buf[++n] = '\0';

	close(datafd);
	get_reply(controlfd, lbuf, sizeof(lbuf), 1);	/* Should be '226 Transfer complete' */
	return 1;
}
		


int do_ls(int controlfd, int datafd, char *input, int mode) {
	char filelist[MAXPATHLEN], recvline[IOBUFLEN+1];
	fd_set rdset;
	int n, maxfdp1, data_finished = FALSE, control_finished = FALSE;

	bzero(filelist, sizeof(filelist));
	if (atype != ASCII) settype(controlfd, ASCII);

	if (get_parm(input, filelist) < 0) 
		sprintf(recvline, "LIST\r\n");
	else
		sprintf(recvline, "LIST %s\r\n", filelist);

	send_cmd(controlfd, recvline);
	if (mode == PORT) {	/* active mode, wait for connection */
		if ((n = dataconn(datafd)) < 0) {
			printf("Active mode connect failed.\n");
			return -1;
		}
		datafd = n;
	}

	FD_ZERO(&rdset);

	maxfdp1 = MAX(controlfd, datafd) + 1;

	while (1) {
		if (control_finished == FALSE) FD_SET(controlfd, &rdset);
		if (data_finished == FALSE)  FD_SET(datafd, &rdset);
		select(maxfdp1, &rdset, NULL, NULL, NULL);

		if (FD_ISSET(controlfd, &rdset)) {
			get_reply(controlfd, recvline, IOBUFLEN, 1);
			if (*recvline != '1') {	/* 150 Opening ASCII mode connecion ... */
				printf("Unexpected response: %s", recvline);
				break;
			}
			control_finished = TRUE;
			//bzero(recvline, (int)sizeof(recvline));
			FD_CLR(controlfd, &rdset);
		}

		if (FD_ISSET(datafd, &rdset)) {
			while ((n = read(datafd, recvline, IOBUFLEN)) > 0) {
				//printf("%s", recvline); 
				//bzero(recvline, (int)sizeof(recvline)); 
				write(1, recvline, n);
			}
			data_finished = TRUE;
			FD_CLR(datafd, &rdset);
		}
		if ((control_finished == TRUE) && (data_finished == TRUE)) {
			get_reply(controlfd, recvline, IOBUFLEN, 1);
			if (*recvline != '2') 	/* 226 Transfer Complete */
				printf("DIR: Transfer error: %s\n", recvline);
			break;
		}
	}
	return 1;
}

int do_get(int controlfd, char *file1, char *file2, int mode) {
	char iobuf[IOBUFLEN+1];
	int status = 1, bcnt = 0, fd, n, datafd = -1, icount = 0;
	int maxfdp1, data_finished = FALSE, control_finished = FALSE;
	fd_set rdset;

	//bzero(iobuf, sizeof(iobuf));
	if (*file2 == '\0') file2 = file1;

	if ((fd = open(file2, O_WRONLY|O_TRUNC|O_CREAT, 0664)) < 0) {
		printf("GET: Cannot open local file '%s'\n", file2);
		return -1;
	}
	if (atype != type) settype(controlfd, type);
#ifdef BLOATED
	if (mode != PASV)
		datafd = do_active(controlfd);
	else
#endif
		datafd = do_passive(controlfd);
	if (datafd < 0) return -1;

	sprintf(iobuf, "RETR %s\r\n", file1);
	send_cmd(controlfd, iobuf);

	get_reply(controlfd, iobuf, IOBUFLEN, 1);
	if (*iobuf != '1') {	/* Any error on the remote side will emit a 5xx response */
		if (!debug) printf("%s", iobuf);
		status = -1;
		goto get_out;
	}

	if (mode == PORT) {	/* active mode, wait for connection */
		if ((n = dataconn(datafd)) < 0) {
			printf("Active mode connect failed: %d.\n", errno);
			status = -2;	// Fatal error
			goto get_out;
		}
		datafd = n;
	}
	if (datafd < 0) goto get_out;

	FD_ZERO(&rdset);

	maxfdp1 = MAX(controlfd, datafd) + 1;
	printf("remote: %s, local: %s\n", file1, file2);
	/*
	 * NOTICE: When fetching very small or NULL size files, the '150 Opening'
	 * message and the 226 Transfer Complete message arrive almost at the same time.
	 * So when get_reply() fetches the former, the latter ends up in the input buffer,
	 * and the loop below will not receive anything on the control channel.
	 */
	struct timeval tv; int select_return;
	while (1) {
		if (control_finished == FALSE) FD_SET(controlfd, &rdset);
		if (data_finished == FALSE) FD_SET(datafd, &rdset);
		tv.tv_sec  = 0;
		tv.tv_usec = 1000;	//Experimental
		select_return = select(maxfdp1, &rdset, NULL, NULL, &tv);
/// Experimental
		if (!select_return) { 
			if (debug > 2) printf("get: select timeout.\n");
			if (icount++ > 2 || data_finished == TRUE) {	
				//Experimental: Got timeout, need reply - which is probalby sitting in the
				// fgets() input buffer.
				// If it isn't, we're stuck here.
				get_reply(controlfd, iobuf, IOBUFLEN, 1);
				break;
			}
		}

		if (FD_ISSET(controlfd, &rdset)) {
			get_reply(controlfd, iobuf, IOBUFLEN, 1);
			//printf("get select cntrl\n");
			if (*iobuf != '2') {
				printf("Unexpected: %s" , iobuf);
				continue;
			}
			control_finished = TRUE;
			FD_CLR(controlfd, &rdset);
		}

		if (FD_ISSET(datafd, &rdset)) {
			while ((n = read(datafd, iobuf, IOBUFLEN)) > 0) {
				if (write(fd, iobuf, n) != n) {
					perror("File write error");
					break;
					// MAY have to reset data connection 
				}
				bcnt += n;
			}
			//printf("got %d\n", bcnt);	// For later use
			data_finished = TRUE;
			FD_CLR(datafd, &rdset);
		}
		if ((control_finished == TRUE) && (data_finished == TRUE))
			break;

	}
get_out:
	if (datafd >= 0) close(datafd);
	close(fd);
	return status;
}

int do_put(int controlfd, char *file1, char *file2, int mode){
	char iobuf[IOBUFLEN+1];
	int datafd, fd, n, status = 1;

	bzero(iobuf, sizeof(iobuf));

	if (atype != type) settype(controlfd, type);
	if (*file2 == '\0') file2 = file1;
#ifdef BLOATED
	if (mode != PASV)
		datafd = do_active(controlfd);
	else
#endif
		datafd = do_passive(controlfd);
	if (datafd < 0) return -1;

	if ((fd = open(file1, O_RDONLY)) < 0) {	/* FIXME: we've checked this already ... */
		printf("MPUT: Cannot open local file: %s\n", file1);
		return -1;
	}

	sprintf(iobuf, "STOR %s\r\n", file2);
	send_cmd(controlfd, iobuf);
	printf("local: %s, remote: %s\n", file1, file2);

	get_reply(controlfd, iobuf, IOBUFLEN, 1);
	
	if (*iobuf != '1') {	/* Any error on the remote side will emit a 5xx response */
		if (!debug) printf("%s", iobuf);	// Make sure the user gets the error
		status = -1;
		goto put_out;
	}

	if (mode == PORT) {	/* active mode, wait for connection */
		if ((n = dataconn(datafd)) < 0) {
			printf("Active mode connect failed.\n");
			return -1;
		}
		datafd = n;
	}

#if 0
	//bzero(iobuf, sizeof(iobuf));
	//get_reply(controlfd, iobuf, IOBUFLEN, 1);	/* get '150 Opening connection ...' */
	if (*iobuf != '1') {
		printf("PORT command failed, %s", iobuf);
		close(datafd);
		return -1;
	}
#endif

#if PUTSELECT
	int maxfdp1, data_finished = FALSE, control_finished = FALSE;
	fd_set wrset, rdset;
	FD_SET(controlfd, &rdset);
	FD_SET(datafd, &wrset);

	maxfdp1 = MAX(controlfd, datafd) + 1;

	// FIXME: Using select does not make sense until this code is rearranged
	// so that the control connection is checked while the transfer is ongoing
	// to accomodate transfer abort.
	while (1) {
		if (control_finished == FALSE) FD_SET(controlfd, &rdset);
		if (data_finished == FALSE) FD_SET(datafd, &wrset);
		select(maxfdp1, &rdset, &wrset, NULL, NULL);

		if (FD_ISSET(controlfd, &rdset)) {
			get_reply(controlfd, iobuf, IOBUFLEN, 1);
			// FIXME - improve error handling
			if (*iobuf != '2' && !debug) {
				printf("%s", iobuf);
				//break;
			}
			control_finished = TRUE;
			FD_CLR(controlfd, &rdset);
		}

		if (FD_ISSET(datafd, &wrset)) {
			bzero(iobuf, (int)sizeof(iobuf));
			while ((n = read(fd, iobuf, IOBUFLEN)) > 0) {
				write(datafd, iobuf, n);	// FIXME: NO error checking
			}
			close(datafd);
			data_finished = TRUE;
			FD_CLR(datafd, &wrset);
		}
		if ((control_finished == TRUE) && (data_finished == TRUE)) {
			break;
		}
	}
#else
	while ((n = read(fd, iobuf, IOBUFLEN)) > 0) {
		write(datafd, iobuf, n);	// FIXME: NO error checking
	}
	close(datafd);
	get_reply(controlfd, iobuf, IOBUFLEN, 1);
#endif

put_out:
	close(fd);
	return status;
}

#ifdef BLOATED
/* 
 * Active mode - PORT command: 
 * Open a port for the server to connect to 
 */
int do_active(int cmdfd) {
	struct sockaddr_in myaddr;
	int fd, i = 0;
	unsigned int myport;
	char str[BUF_SIZE], *p, *myip;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}
#if 0
	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
		/* This should not happen */
		printf("Data channel socket error: REUSEADDR, closing channel.\n");
		close(fd);
		return -1;
	}
#endif

	i = SO_LISTEN_BUFSIZ;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0) 
		perror("SO_RCVBUF");

	bzero(&myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);	/* system picks a port */
	i = sizeof(myaddr);

	if (bind(fd, (struct sockaddr *) &myaddr, i) < 0) {
		perror("bind error");
		return -1;
	}

	if (getsockname(fd, (struct sockaddr *)&myaddr, (unsigned int *)&i) < 0) {
		perror("getsockname");
		close(fd);
		return -1;
	}
 	myip = in_ntoa(myaddr.sin_addr.s_addr);
	myport = ntohs(myaddr.sin_port);
	p = (char *)&myaddr.sin_port;
	if (debug > 1) printf("Listening on %s port %u\n", myip, myport);

	if (listen(fd, 1) < 0) {
		perror("Listen failed");
		close(fd);
		return -1;
	}

	get_port_string(str, myip, (int) UC(p[0]), (int) UC(p[1]));
	send_cmd(cmdfd, str);

	if (get_reply(cmdfd, str, BUF_SIZE, 1) < 0) {	// 200 Port command successful ...
		/* command channel error, probably server timeout */
		close(fd);
		fd = -1;
	}
	return fd;
}
#endif /*BLOATED*/

/* PASSIVE MODE:
 * Connect to service point announced by server */
int do_passive(int cmdfd) {
	
	struct sockaddr_in srvaddr;
	int fd = 0;
	unsigned int port;
	char command[BUF_SIZE], ip_addr[ADDRBUF];
	//FILE *cmd;

	send_cmd(cmdfd, "PASV\r\n");
	get_reply(cmdfd, command, BUF_SIZE, 1);
	if (strncmp(command, "227", 3)) {
		printf("Server error, cannot connect.\n");
		return -1;
	}
	
	if (get_client_ip_port(command, ip_addr, &port) < 0) {
		printf("Passive mode error: %s", command);
		return -1;
	}
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			perror("socket error");
			return -1;
	}

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr*) &srvaddr, sizeof(srvaddr)) < 0) {
		perror("bind error");
		return -1;
	} 

	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = in_aton(ip_addr);
	srvaddr.sin_port = htons(port);

	if (debug > 1) printf("Connecting to %s @ %u\n", ip_addr, port);

	if (connect(fd, (struct sockaddr *) &srvaddr, sizeof(srvaddr)) < 0) {
			perror("connect error");
			return -1;
	}
	return fd;
}

int do_mput(int controlfd, char *mdir, int mode) {
	DIR *dir = NULL;
	struct dirent *dp;
	struct stat buf;
	char iobuf[IOBUFLEN+1];
	int l, i, filecount = 0, count = 0;
	
#ifdef GLOB
	char *myargv[MAXARGS];

	if (glob) count = expandwildcards(mdir, MAXARGS, myargv);
	//FIXME: Simplify this by having expandwildcards() return mdir as a single arg
	if (!count) {
#endif
		// Assume argument is a dir, FIXME when we support multiple args.
	    	dir = opendir(mdir); 	/* test for existence */
    		if (!dir) {
    			printf("Directory not found: %s\n", mdir);
    			return -1;
    		} 
		*iobuf = '\0';
		// If no wildcards, we're assuming the parameter is a directory.
		// This is nonstandard. The normal is to not accept a directory and
		// expect a series of filename arguments when globbing is off.

		// FIXME: The test for directories may be superfluous, let do_put handle it.
		while ((dp = readdir(dir)) != NULL) {
			stat(dp->d_name, &buf);
			if ((*dp->d_name != '.') && !S_ISDIR(buf.st_mode) && (l = ask("mput", dp->d_name)) == 0) {
				if (do_put(controlfd, dp->d_name, "", mode) < 0) break;
				filecount++;
			}
			if (l < 0) break;
		}
		closedir(dir); 
#ifdef GLOB
	} else { 	/* wildcard list */
		for (i = 0; i < count; i++) {
			if ((l = ask("mput", myargv[i])) == 0 ) {
				if (do_put(controlfd, myargv[i], "", mode) < 0) break;
				filecount++;
			}
			if (l < 0) break;
		}
	} 
	freewildcards();
#endif
	printf("mput: %d files transferred.\n", filecount);
	return 1;
}

int do_mget(int controlfd, char *rdir, int mode) {
	char lbuf[NLSTBUF+1];
	char *name, *next = NULL;
	int n, l, filecount = 0;

	bzero(lbuf, NLSTBUF);
	if (!glob) {
		printf("Globbing is off and multiple arguments are not supported.\nmget will return a single file.\n");
		name = rdir;
	} else {
		if (do_nlst(controlfd, lbuf, NLSTBUF, rdir, mode) < 1) 
			return -1;
		name = lbuf;
		*(next = strchr(lbuf, '\r')) = '\0';
	}
	while (name != NULL) {
		if ((l = ask("mget", name)) == -1) break;
		next += 2;
		if (!l) {
			//if (prompt == FALSE) printf("remote: %s, local: %s\n", name, name);
			if ((n = do_get(controlfd, name, "", mode)) < -1) 
				break;	// Error -2 is fatal, -1 = this file failed.
			if (n > 0) filecount++;
		}
		if (!glob) break;	// FIXME: when multiple args supported
		name = next;
		next = strchr(next, '\r');
		if (next == NULL) break;
		*next = '\0';
	}
	if (filecount > 1) printf("mget: %d files transferred.\n", filecount);
	return 1;
}

void settype(int controlfd, char t) {
	char b[ADDRBUF];

	/* atype - active type (what we've told the server)
	 * type - selected file transfer type (via BIN or ASCII commands)
	 */
	if (atype == t) return;
	sprintf(b, "TYPE %c\r\n", t);
	send_cmd(controlfd, b);

	get_reply(controlfd, b, ADDRBUF, 1);	/* Should be '200 Type set to ...' */
	if (*b != '2') 
		printf("Failed to set type: %s", b);
	else 
		atype = t;
	return;
}

int connect_cmd(char *ip, unsigned int server_port) {
	struct sockaddr_in servaddr;
	int controlfd;

	if ((controlfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}
	//printf("CMD socket OK.\n");

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(INADDR_ANY);
	servaddr.sin_addr.s_addr = in_aton(0);

	if (bind(controlfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind");
		return -2;
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(server_port);
	servaddr.sin_addr.s_addr = in_aton(ip);	

	if (debug > 1) printf("Connecting to %s @ port %u\n", ip, server_port);
	if (connect(controlfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		perror("connect error");
		return -1;
	}
	return controlfd;
}

void do_close(int controlfd, char *str, int len) {
	if (send_cmd(controlfd, "QUIT\r\n") < 0) 
		printf("Server has closed connection.\n");
	else {
		if (get_reply(controlfd, str, len, 1) < 0)
			sprintf(str, "Server timed out.\n");
		printf("Closing: %s", str);
	}
	return;
}

int do_login(char *user, char *passwd, char *buffer, int len, char *ip, unsigned int port) {
	char localbf[50], *lip;
	int controlfd = -1;

	lip = ip;
	if (*lip == '\0') {	/* ask for server address */
		printf("Connect to: ");	//FIXME: must be able to specify port # too 
		lip = gets(localbf);
		if (!lip) return -1;
		strcpy(ip, lip);
	}
	if ((controlfd = connect_cmd(ip, port)) < 0)
		return -1;
	printf("Connected to %s.\n", ip);
	fcmd = fdopen(controlfd, "r+");

	/* Get the ID message from the server */
	get_reply(controlfd, buffer, len, 0);	/* 220 XYZ ftp server ready etc. */

	/* Do the login process */

	//printf("name '%s', pw '%s'\n", user, passwd);
	printf("Name (%s:%s): ", ip, user);
	if (strlen(gets(localbf)) > 2) strcpy(user, localbf); /* no boundary checking */
	sprintf(buffer, "USER %s\r\n", user);
	send_cmd(controlfd, buffer);
	get_reply(controlfd, buffer, len, 1);	/* 331 Password required ... */
	if (*buffer != '3') {
		printf("Error in username: %s", buffer);
		return -1;
	}
	lip = getpass("Password:");
	printf("\n");
	if (strlen(lip) < 2) lip = passwd; /* use passwd from command line if present */
	sprintf(buffer, "PASS %s\r\n", lip);
	send_cmd(controlfd, buffer);

	/* Gobble up welcome message - line by line to catch continuation */
	while (fgets(buffer, len, fcmd) != NULL) {
		printf("%s", buffer);
		if (buffer[3] == ' ') break;
	}
	if (*buffer != '2') {	/* 230 User xxx logged in. */
		printf("Login failed: %s", buffer);
		return -1;
	}
	//printf("\nLogin OK.\n");
	send_cmd(controlfd, "SYST\r\n");
	get_reply(controlfd, buffer, len, 1);
	if (strstr(buffer, "UNIX")) {
		printf("Remote system is Unix/Linux, using binary mode\n");
		type = BIN;
	} else printf("File transfer mode is ASCII\n");
	return controlfd;
}

int main(int argc, char **argv) {

	int server_port = 21, controlfd = -1, datafd, code, connected = 0;
	int mode = PASV, mput, mget, f_mode, autologin = 1;
	char command[BUF_SIZE], ip[ADDRBUF], str[IOBUFLEN+1], user[30], passwd[30];
	char param[MAXPATHLEN];

	strcpy(user, "ftp");
	*passwd = '\0';
	*ip = '\0';

	while (argc > 1) {
		argc--;
		argv++;
		if (**argv == '-') {
			switch ((*argv)[1]) {
			case 'u':
				strcpy(user, argv[1]);
				break;
			case 'p':
				strcpy(passwd, argv[1]);
				break;
#ifdef BLOATED
			case 'n':
				autologin = 0;
				break;
			case 'P':
				mode = PASV;
				break;
			case 'A':
				mode = PORT;
				break;
			case 'v':
			case 'd':
				debug++; // Multiple -d increases debug level
				break;
			case 'i':
				prompt = FALSE;
				break;
#endif
			default:
				printf("Error in arguments.\n");
				exit(-1);
			}
			argv++;
			argc--;
		} else {
			if (strchr(*argv, '.') != 0) strcat(ip, *argv);
			else server_port = atoi(*argv);
		}
	}
	if (*ip == '\0') {
		autologin = 0;
	}

	if (autologin) {
		if ((controlfd = do_login(user, passwd, command, sizeof(command), ip, server_port)) > 0)
			connected++;
	}
	bzero(command, BUF_SIZE);

	while(1) {
		mput = 0; mget = 0;
		f_mode = R_OK;
		if ((code = get_cmd(command, sizeof(command))) < 0) break; /* got EOF */
		
		// prequalify commands so we don't have to do mode checking on every command
		if (!connected && (code > CMD_NOCONNECT) && (code < CMD_CONNECT)) {
			printf("Not connected.\n");
			continue;
		}
		switch (code) {

		case CMD_OPEN:
			if (connected) {	// already connected, check with NOOP
				send_cmd(controlfd, "NOOP\r\n");
				if (get_reply(controlfd, str, IOBUFLEN, 1) < 0) {
					printf("Server has gone away, closing connection.\n");
					close(controlfd);
					connected = 0;
				} else {
					printf("Already connected to %s, use close first.\n", ip);
					break;
				}
			}
			if (get_parm(command, param) > 0) strcpy(ip, param);
			if ((controlfd = do_login(user, passwd, str, sizeof(str), ip, server_port)) < 0) 
				printf("Login failed, connection closed.\n");
			else
				connected++;
			break;

		case CMD_CLOSE:
			do_close(controlfd, str, sizeof(str));
			connected = 0;
			break;

		case CMD_MPUT:
			mput++;
			f_mode = X_OK;
			/* fall thru */

		case CMD_PUT:
			if (get_parm(command, param) < 0) break;
#ifndef GLOB
			if (*param == '*') *param = '.';  /* No globbing, full directory transfer */
			if (access(param, f_mode) < 0) {
				perror(param);
				break;
			}
#else
			code = f_mode; /* silence the compiler */
#endif
			if (mput)
				do_mput(controlfd, param, mode);
			else 
				do_put(controlfd, param, "", mode);
			break;


		case CMD_MGET:
			mget++;
			/* fall thru */

		case CMD_GET:
			if (get_parm(command, param) < 0) break;
			if (mget) {
				if (do_mget(controlfd, param, mode) < 0) 
					printf("MGET error.\n");
			} else 
				do_get(controlfd, param, "", mode);
			break;

		case CMD_DIR:
			if (type != ASCII) settype(controlfd, ASCII);
#ifdef BLOATED
			if (mode != PASV)
				datafd = do_active(controlfd);
			else
#endif
				datafd = do_passive(controlfd);
			if (datafd > 0)
				if (do_ls(controlfd, datafd, command, mode) < 0) {
					printf("DIR error\n");
				}
			close(datafd);
			break;

		case CMD_CWD:
			if (get_parm(command, param) < 0) {
				printf("Command needs parameter.\n");
				break;
			}
			sprintf(str, "CWD %s\r\n", param);
			send_cmd(controlfd, str);
			get_reply(controlfd, command, BUF_SIZE, 0);
			break;
			
		case CMD_PWD:
			send_cmd(controlfd, "PWD\r\n");
			get_reply(controlfd, command, BUF_SIZE, 0);
			break;

		case CMD_QUIT:
			do_close(controlfd, command, sizeof(command));
			connected = 0;
			goto out;

#ifdef BLOATED
		case CMD_SHELL:
			system(&command[1]);
			break;
			
		case CMD_DELE:
			if (get_parm(command, param) < 0) {
				printf("Command needs parameter.\n");
				break;
			}
			sprintf(str, "DELE %s\r\n", param);
			send_cmd(controlfd, str);
			get_reply(controlfd, command, BUF_SIZE, 0);
			break;

		case CMD_MKD:
			if (get_parm(command, param) < 0) {
				printf("Command needs parameter.\n");
				break;
			}
			sprintf(str, "MKD %s\r\n", param);
			send_cmd(controlfd, str);
			get_reply(controlfd, command, BUF_SIZE, 0);
			break;

		case CMD_HELP:
			help();
			break;

		case CMD_LCWD:
			if (get_parm(command, param) < 0) {
				printf("Command needs parameter.\n");
				break;
			}
			if (chdir(param) < 0) {
				perror(param);
			} else {
				getcwd(str, IOBUFLEN);
				printf("local directory now %s\n", str);
			}
			break;
			
		case CMD_VERB:
			if (debug) {
				debug = 0;
				printf("Verbose mode off.\n");
			} else {
				debug = 1;
				printf("Verbose mode on.\n");
			}
			break;


		case CMD_STATUS:
			// include the server ID line here
			if (connected) {
				printf("Connected to %s [port %d] as user %s\n", ip, server_port, user);
				send_cmd(controlfd, "SYST\r\n");
				get_reply(controlfd, str, sizeof(str), 10);
				printf("Remote system: %s", &str[4]);
			}
			else printf("Not connected.\n");
			if (!debug) printf("Debug is off.\n");
			else printf("Debug: %d\n", debug);
			printf("Globbing: %s\n", glob ? "on" : "off");
			printf("Transfer mode: %s\n", (mode == PASV) ? "passive" : "active");
			printf("File mode: %s\n", (type == ASCII) ? "ASCII" : "IMAGE");
			printf("Multifile prompting is %s\n", prompt ? "on" : "off");
			// add file count, byte count etc. later.
			printf("\n");
			break;

		case CMD_PASV:
			if (mode == PASV) {
				mode = PORT;
				printf("Passive mode off.\n");
			} else {
				mode = PASV;
				printf("Passive mode on.\n");
			}
			break;
			
		case CMD_GLOB:
			glob++;
			glob &= 1;
			printf("Globbing %s.\n", glob ? "on" : "off");
			break;
#endif
			
		case CMD_BIN:
			type = BIN;
			printf("Type is binary.\n");
			break;
			
		case CMD_ASCII:
			type = ASCII;
			printf("Type is ascii.\n");
			break;

		case CMD_PROMPT:
			printf("Interactive mode ");
			if (prompt == TRUE) {
				printf("off.\n");
				prompt = FALSE;
			} else {
				printf("on.\n");
				prompt = TRUE;
			}
			break;

		case CMD_DEBUG:
			if (get_parm(command, param) < 0)
				debug++;
			else {
				if (isdigit(*param))
					debug = atoi(param);
				else {
					if (!strcmp(param, "off")) debug = 0;
					else printf("Illegal parameter.\n");
				}
			}
			if (debug) printf("Debug is on (%d).\n", debug);
			else printf("Debug is off.\n");
			break;

		case CMD_TYPE:
			printf("Using %s mode to transfer files.\n", (type == ASCII) ? "ascii" : "binary");
			break;
#ifndef BLOATED
		case CMD_SHELL:
		case CMD_HELP:
		case CMD_VERB:
		case CMD_LCWD:
		case CMD_DELE:
		case CMD_MKD:
		case CMD_STATUS:
		case CMD_GLOB:
		case CMD_PASV:
#endif
		case CMD_USER:
		case CMD_RMD:
			printf("Not implemented.\n");
			break;

		case CMD_NULL:
		default:
			printf("No such command.\n");
			break;
		}
	}
out:
	if (connected) close(controlfd);	
	return TRUE;
}
