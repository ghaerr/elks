/*
 *
 * Minimal FTP client for ELKS 
 * by Helge Skrivervik - helge@skrivervik.com - December 2021
 *
 *	TODO:
 *	- add missing commands (rmdir, mls, nlist, ...), 
 *	- ... create common function for commands that are alomost alike
 *	- fix trucation of file names in FAT, minix (warnings, name collisions)
 *	- lcd with no params should print current local directory
 *	- add ABORT support in PUT
 *	- handle server timeout (no activity): Unsolicited server input (code 421)
 *	- add '15632 bytes sent in 0.00 secs (27.5561 MB/s)' type message after transfer
 */

#define BLOATED		/* fully featured if defined */
#ifdef  BLOATED
#define GLOB		/* include wildcard processing */
#endif
#define	QEMUHACK	/* Include code for QEMU special treatment */

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
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#ifdef GLOB
#include <pwd.h>
#include <regex.h>
#endif


#define	IOBUFLEN 1500
#define BUF_SIZE 512
#define CMDBUF 80
#define ADDRBUF	40
#define NLSTBUF 1500
#define MAXARGS	256	/* max names in wildcard expansion */
#define MAXPARMS 30	/* reasonable max # of internal cmdline parameters */
#define ALRMINTVL 10
#define DFLT_PORT 21

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
static int send_cmd(int, char *);
static int is_connected(int);
static int parse_cmd(char *, char **);

//static FILE *fcmd;

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
	CMD_PORT,
	CMD_PASV,
	CMD_SITE,
	CMD_CONNECT,	// flag
	// commands in any mode
	CMD_QUIT,
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
	{"ascii", CMD_ASCII, "Set file transfer type to ASCII"},
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
	{"site", CMD_SITE, "Send command to server - supports 'idle'"},
#endif
	//{"user", CMD_USER, "Open new connection, log in."},
	{"", CMD_NULL, ""}
};
static int debug = 1;
static char type = ASCII, atype = ASCII;
static int prompt = TRUE, glob = TRUE;
static int check_connected = 0;
static int connected = 0;
static char srvr_ip[ADDRBUF], myip[ADDRBUF]; 	/* For qemu hack */

#ifdef QEMUHACK
static int qemu = 0;
#endif

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
		
#if 0
/* Read (and echo) reply from server */
/* NOTE: The fd argument is not used (replaced by global fcmd for buffered IO). */
/* fd is kept because it makes the code easier to read. */
int get_reply(int fd, char *buf, int size, int dbg) {

	if (fgets(buf, size, fcmd) == NULL) {
		check_connected++;
		return -1;
	}
	/* dbg - at which debug level this command should be echoed */
	if (debug >= dbg) printf("%s", buf);
	return 1;
}
#else
/*
 * get_reply: Return exactly one complete reply from the server.
 * - Collect long (multiline) replies
 * - Buffer replies if several arrive concurrently (never more than one extra)
 * - Return buffer status when queried
 *
 * This version of get_reply eliminates the use of buffered IO in order for select() to work predictably
 * and enable querying of possible buffered replies (the peculiarities created by the huge difference in speed
 * between and ELKS system and a more recent system).
 * E.g when receiving small (and zero length) files in particular, the 'Transfer Complete' control message 
 * will arrive before any data and be read as part of the previous command response.
 * Multiline control responses (typically at login), will some times arrive in several packets and must be
 * assembled properly.
 * Not elegant, but very memory efficiant (abusing the *buf parameter for all the work).
 * DO NOT call get_reply with a buf size less than BUF_SIZ (512b).
 */

int get_reply(int fd, char *buf, int size, int dbg) {
	static char lbuf[CMDBUF];
	static int lb = 0;
	
	char *cp;
	int l = 0;

	if (dbg < 0) 		/* Just return status */
		return lb;

	if (lb) {		/* something in the buffer, return it */
		strncpy(buf, lbuf, size);
		if (debug >= dbg) printf("(bf) %s", buf);
		if (!strncmp(buf, "421", 3)) connected = 0;	/* look for server timeout */
		lb = 0;
		return 1;
	}
	bzero(buf, size);

	while (1) {	/* Need this hack to handle multi-line replies which may */
			/* arrive in several packets and some times require several reads. */
		if ((l = read(fd, &buf[lb], size-lb)) <= 0) {
			check_connected++;
			lb = 0;
			return -1;
		}	// FIXME: Should check whether we're actually running out of buffer space
		buf[l+lb] = '\0';

		// find the start of the last received line 
		cp = strrchr(&buf[lb], '\n') - 1;
		while (*cp != '\n' && cp >= &buf[lb]) --cp;
		cp++;
		lb += l;
		if (*(cp+3) != '-') 	/* end of continuation if blank */
			break;
	}
	buf[lb] = '\0';
	lb = 0;

	// cp points to the start of the last record,
	// compare status codes to see if we have an extra
	// reply to save for the next call.

	if (cp != buf) {
		if (strncmp(buf, cp, 3)) {
			strncpy(lbuf, cp, sizeof(lbuf));	// save the extra reply
			lb = strlen(cp);
			*cp = '\0';
			//printf("\nreturned <%s>\nsaved <%s>\n", buf, lbuf);
		}
	}
	
	if (!strncmp(buf, "421", 3)) connected = 0;
	if (debug >= dbg || !connected) printf("%s", buf); // If the server timed out, always
							   // print the status message.
	return 1;
}

#endif

int is_connected(int cmdfd) {	// return 1 if connected, otherwise 0
	char str[IOBUFLEN];

	if (cmdfd < 0 || !connected) 
		return 0;

	if (send_cmd(cmdfd, "NOOP\r\n") < 0) {
		close(cmdfd);
		return 0;
	}
	return ((get_reply(cmdfd, str, sizeof(str), 1) > 0)? 1 : 0 );
}

/* Send command to server, echo if required */

int send_cmd(int cmdfd, char *cmd) {
	int s;

	s = write(cmdfd, cmd, strlen(cmd));
	if (s < 0)
		check_connected++;
	else
		if (debug > 1) printf("---> %s", cmd);
	return s;
}
	
/* Read and process user commands */

int get_cmd(char *buffer, int len, char **argv) {
	struct cmd_tab *c = cmdtab;
	int cmdlen;
	char *cmd;

	do {
		printf("ftp> ");
		if (fgets(buffer, len, stdin) == NULL)
			return -1;
		if (*buffer == '\4') return -1; // ^D
	} while (strlen(buffer) < 2);

	if (*buffer == '!') return CMD_SHELL;
	if (*buffer == '?') return CMD_HELP;

	parse_cmd(buffer, argv);
	cmd = *argv;
	cmdlen = strlen(cmd);

	while (*c->cmd != '\0') {
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

/* Decipher the PASV info from the server - only called from do_passive() */

int get_server_ip_port(char *str, char *server_ip, unsigned int *server_port) {
	char *n[6];
	int p5 = 0, p6;

	if (strtok(str, "(") == NULL)
		return -1;
	while (p5 < 6)
		n[p5++] = strtok(NULL, ",)");

	sprintf(server_ip, "%s.%s.%s.%s", n[0], n[1], n[2], n[3]);
	p5 = atoi(n[4]);
	p6 = atoi(n[5]);
	*server_port = (256 * p5) + p6;

	return 1;
}

int get_ip_port(int fd, char *ip, unsigned int *port) {
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	getsockname(fd, (struct sockaddr*) &addr, &len);
	strcpy(ip, in_ntoa(addr.sin_addr.s_addr));
	*port = (unsigned int)ntohs(addr.sin_port);
	//printf("get_ip: %lx %u\n", addr.sin_addr.s_addr, *port);
	return 1;
}

/* Parse command line, return command list and # of args found */

int parse_cmd(char *input, char **argv) {
	char *parm;
	int i = 0;

	trim(input);
	//printf("parse_cmd: <%s>\n", input);
	argv[i++] = strtok(input, " ");
	while ((parm = strtok(NULL, " \r\n\t"))) {
		argv[i++] = parm;
		//printf("parse_cmd: %s\n", parm);
	}
	argv[i] = NULL;
	argv[i+1] = NULL;		// Insurance
	return i;
}


int dataconn(int fd) {		/* wait for the actual data connection in PORT mode */
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
/*        We can call do_nlst several times instead of increasing buffer space. */
/*	  For multifile transfers, the additional time is negligible */

int do_nlst(int controlfd, char *buf, int len, char *dir, int mode) {
	int n, i, datafd;
	char pbuf[BUF_SIZE];

	if (atype != ASCII) settype(controlfd, ASCII);
#ifdef BLOATED
	if (mode != PASV)
		datafd = do_active(controlfd);
	else
#endif
		datafd = do_passive(controlfd);
	if (datafd <= 0) {
		return -1;
	}

	sprintf(pbuf, "NLST %s\r\n", dir);
	send_cmd(controlfd, pbuf);

	if (mode == PORT) {	/* active mode, wait for connection */
		if ((n = dataconn(datafd)) < 0) {
			printf("Active mode connect failed.\n");
			return -1;
		}
		datafd = n;
	}
	get_reply(controlfd, pbuf, sizeof(pbuf), 1);
	if (pbuf[0] != '1') {		/* 125 List started OK */
		printf("Couldn't get remote filelist.\n");
		close(datafd);
		return -1;
	}
	n = 0;
	bzero(buf, len);
	while (n < len-1) {
		if ((i = read(datafd, &buf[n], len-n-1)) <= 0) break;
		if (debug > 3) printf("do_nlst: got %d (%d) bytes\n", n, i);
		n += i;
	}
	if (i > 0) {
		printf("Warning: File list too long, truncated\n");
		n = len - 2;
		while (buf[n] != '\n') n--;
	}
	buf[++n] = '\0';
	//printf("NLST: %d (%d) %s<<<\n", n, strlen(buf), buf);
	close(datafd);
	get_reply(controlfd, pbuf, sizeof(pbuf), 1);
	return 1;
}
		
/* Implements the dir command */
int do_ls(int controlfd, int datafd, char **cmdline, int mode) {
	char recvline[IOBUFLEN+1];
	fd_set rdset;
	int nfd, n, maxfdp1, data_finished = FALSE, control_finished = FALSE;
	int fd = 1; 	// Defaults to stdout

	if (atype != ASCII) settype(controlfd, ASCII);

	if (cmdline[1])
		sprintf(recvline, "LIST %s\r\n", cmdline[1]);
	else
		sprintf(recvline, "LIST\r\n");
	if (cmdline[2]) {					// save output in file
		if ((fd = open(cmdline[2], O_WRONLY|O_TRUNC|O_CREAT, 0664)) < 0) {
			printf("DIR: Cannot open output file '%s'\n", cmdline[2]);
			return -1;
		}
	}
	if (send_cmd(controlfd, recvline) < 0) {
		printf("No connection.\n");
		check_connected++;
		if (fd > 1) close(fd);
		return -1;
	}
	if (mode == PORT) {	/* active mode, wait for connection */
		if ((nfd = dataconn(datafd)) < 0) {
			printf("Active mode connect failed.\n");
			return -1;
		}
	} else
		nfd = datafd;

	FD_ZERO(&rdset);

	maxfdp1 = MAX(controlfd, nfd) + 1;

	while (1) {
		if (control_finished == FALSE) FD_SET(controlfd, &rdset);
		if (data_finished == FALSE)  FD_SET(nfd, &rdset);
		select(maxfdp1, &rdset, NULL, NULL, NULL);

		if (FD_ISSET(controlfd, &rdset)) {
			get_reply(controlfd, recvline, sizeof(recvline), 1);
			if (*recvline != '1') {	/* 150 Opening ASCII mode connecion ... */
				printf("Unexpected response: %s", recvline);
				break;
			}
			control_finished = TRUE;
			//bzero(recvline, (int)sizeof(recvline));
			FD_CLR(controlfd, &rdset);
		}

		if (FD_ISSET(nfd, &rdset)) {
			while ((n = read(nfd, recvline, IOBUFLEN)) > 0) {
				write(fd, recvline, n);
			}
			data_finished = TRUE;
			FD_CLR(nfd, &rdset);
		}
		if ((control_finished == TRUE) && (data_finished == TRUE)) {
			get_reply(controlfd, recvline, sizeof(recvline), 1);
			if (*recvline != '2') 	/* 226 Transfer Complete */
				printf("DIR: Transfer error: %s\n", recvline);
			break;
		}
	}
	if (fd > 1) {
		printf("Listing saved in %s\n", cmdline[2]);
		close(fd);
	}
	close(nfd);
	return 1;
}

int do_get(int controlfd, char *src, char *dst, int mode) {
	char iobuf[IOBUFLEN+1];
	int status = 1, bcnt = 0, fd, n, datafd = -1;
	int maxfdp1, data_finished = FALSE, control_finished = FALSE;
	fd_set rdset;

	if (dst == NULL) dst = src;

	if ((fd = open(dst, O_WRONLY|O_TRUNC|O_CREAT, 0664)) < 0) {
		printf("GET: Cannot open/create '%s'\n", dst);
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

	sprintf(iobuf, "RETR %s\r\n", src);
	send_cmd(controlfd, iobuf);

	get_reply(controlfd, iobuf, sizeof(iobuf), 1);
	if (*iobuf != '1') {	/* Any error on the remote side will emit a 5xx response */
		if (!debug) printf("%s", iobuf);
		status = -1;
		goto get_out;
	}

	if (mode == PORT) {	/* active mode, wait for connection */
		if ((n = dataconn(datafd)) < 0) {
			perror("Active mode connect failed");
			status = -2;	// Fatal error
			goto get_out;
		}
		datafd = n;
	}
	if (datafd < 0) goto get_out;

	FD_ZERO(&rdset);

	maxfdp1 = MAX(controlfd, datafd) + 1;
	printf("remote: %s, local: %s\n", src, dst);
	/*
	 * NOTICE: When fetching very small or NULL size files, the '150 Opening'
	 * message and the 226 Transfer Complete message arrive almost at the same time.
	 * When get_reply() fetches the former, the latter ends up in the input buffer,
	 * and the loop below will not receive anything on the control channel.
	 * This problem is now handled in the new get_reply function, which may be poked to see
	 * if there's data pending.
	 */
	struct timeval tv; int select_return, icount = 0;
	long usec = 500;

	while (1) {
		if (control_finished == FALSE) FD_SET(controlfd, &rdset);
		if (data_finished == FALSE) FD_SET(datafd, &rdset);
		tv.tv_sec  = 0;
		tv.tv_usec = usec;
		if (icount < 5 ) usec *= 2;

		select_return = select(maxfdp1, &rdset, NULL, NULL, &tv);

		if (!select_return) { 
			if (debug > 2) printf("get: select timeout (%d)\n", icount);

			// Handle zero length/small files
			if (icount++ > 2 || data_finished == TRUE) {	
				// Got timeout, the reply is either delayed or has arrived already.
				// If a file is zero length, the server will have closed the connection
				// even before we get here.
				if (control_finished == FALSE && (get_reply(controlfd, NULL, 0, -1) > 0)) {
					// found reply message in the reply buffer
					get_reply(controlfd, iobuf, sizeof(iobuf), 1);
					control_finished = TRUE;
				}
			}
		}

		if (FD_ISSET(controlfd, &rdset)) {
			if (debug > 2) printf("get: select ctrl\n");
			get_reply(controlfd, iobuf, sizeof(iobuf), 1);
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
		if (icount > 30 || ((control_finished == TRUE) && (data_finished == TRUE)))
			break;

	}
get_out:
	if (datafd >= 0) close(datafd);
	close(fd);
	return status;
}

int do_put(int controlfd, char *src, char *dst, int mode){
	char iobuf[IOBUFLEN+1];
	int datafd, fd, n, status = 1;

	bzero(iobuf, sizeof(iobuf));

	if (atype != type) settype(controlfd, type);
	if (dst == NULL) dst = src;
#ifdef BLOATED
	if (mode != PASV)
		datafd = do_active(controlfd);
	else
#endif
		datafd = do_passive(controlfd);
	if (datafd < 0) return -1;

	if ((fd = open(src, O_RDONLY)) < 0) {	/* FIXME: we've checked this already ... */
		printf("MPUT: Cannot open local file: %s\n", src);
		return -1;
	}

	sprintf(iobuf, "STOR %s\r\n", dst);
	send_cmd(controlfd, iobuf);
	printf("local: %s, remote: %s\n", src, dst);

	get_reply(controlfd, iobuf, sizeof(iobuf), 1);
	
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

#if PUTSELECT
	int maxfdp1, data_finished = FALSE, control_finished = FALSE;
	fd_set wrset, rdset;
	FD_SET(controlfd, &rdset);
	FD_SET(datafd, &wrset);

	maxfdp1 = MAX(controlfd, datafd) + 1;

	// FIXME: Add support for ABORTing a transfer
	while (1) {
		if (control_finished == FALSE) FD_SET(controlfd, &rdset);
		if (data_finished == FALSE) FD_SET(datafd, &wrset);
		select(maxfdp1, &rdset, &wrset, NULL, NULL);

		if (FD_ISSET(controlfd, &rdset)) {
			get_reply(controlfd, iobuf, sizeof(iobuf), 1);
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
		if (write(datafd, iobuf, n) < n) {
			perror("put");
			break;
		}
	}
#endif
	close(datafd);
	get_reply(controlfd, iobuf, sizeof(iobuf), 1);

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
	char str[BUF_SIZE], port_mode_ip[ADDRBUF], *p;

#ifdef QEMUHACK
	if (qemu) {
		printf("Active mode is not supported in QEMU, use passive mode.\n");
		return -1;
	}
#endif
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		return -1;
	}

	int on = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
		/* This should not happen */
		perror("SO_REUSEADDR");
	}

	i = SO_LISTEN_BUFSIZ;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0) 
		perror("SO_RCVBUF");
	bzero(&myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);	/* if 0, system picks a port */
	i = sizeof(myaddr);

	if (bind(fd, (struct sockaddr *) &myaddr, i) < 0) {
		perror("bind error");
		return -1;
	}

	get_ip_port(fd, port_mode_ip, &myport);
	if (debug > 1) printf("PORT: Listening on %s port %u\n", port_mode_ip, myport);
	p = (char *)&myport;

	if (listen(fd, 1) < 0) {
		perror("Listen failed");
		close(fd);
		return -1;
	}

	get_port_string(str, myip, (int) UC(p[1]), (int) UC(p[0]));
	send_cmd(cmdfd, str);

	if ((get_reply(cmdfd, str, sizeof(str), 1) < 0) || (*str != '2')) { // Should be '200 Port command successful' ...
		/* command channel error, probably server timeout or '551 Rejected data connection' */
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
	char command[BUF_SIZE], *ip, ip_addr[ADDRBUF];

	send_cmd(cmdfd, "PASV\r\n");
	get_reply(cmdfd, command, sizeof(command), 1);
	if (strncmp(command, "227", 3)) {
		return -1;
	}
	
	if (get_server_ip_port(command, ip_addr, &port) < 0) {
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

	ip = ip_addr;
#ifdef QEMUHACK
	if (qemu) ip = srvr_ip;
#endif
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = in_aton(ip);
	srvaddr.sin_port = htons(port);

	if (debug > 1) printf("Connecting to %s @ %u\n", ip, port);

	if (in_connect(fd, (struct sockaddr *) &srvaddr, sizeof(srvaddr), 10) < 0) {
		perror("ftp");
		return -1;
	}
	return fd;
}

int do_mput(int controlfd, char **argv, int mode) {
	DIR *dir = NULL;
	struct dirent *dp;
	struct stat buf;
	char iobuf[IOBUFLEN+1];
	int l, i, filecount = 0, count = 0;
	
	argv++; 			// move past command
	while (*argv) {
#ifdef GLOB
		char *myargv[MAXARGS];

		if (glob) count = expandwildcards(*argv, MAXARGS, myargv);
		if (!count) {
#endif
	    	dir = opendir(*argv); 	/* Accept 1st level directories */ 
    		if (dir) {
			*iobuf = '\0';
			while ((dp = readdir(dir)) != NULL) {
				l = 0;
				stat(dp->d_name, &buf);
				if ((*dp->d_name != '.') && !S_ISDIR(buf.st_mode) && (l = ask("mput", dp->d_name)) == 0) {
					if (do_put(controlfd, dp->d_name, NULL, mode) < 0) break;
					filecount++;
				}
				if (l < 0) break;
			}
			closedir(dir); 
		} else {	// may be just a plain file
			if (access(*argv, F_OK) == 0)
				if (do_put(controlfd, *argv, NULL, mode) > 0) filecount++;
		}
#ifdef GLOB
		} else { 	/* wildcard list */
			for (i = 0; i < count; i++) {
				if ((l = ask("mput", myargv[i])) == 0 ) {
					//FIXME: Test for directories - like above??
					if (do_put(controlfd, myargv[i], NULL, mode) < 0) break;
					filecount++;
				}
				if (l < 0) break;
			}
		} 
		freewildcards();
#endif
		argv++;
	}
	printf("mput: %d files transferred.\n", filecount);
	return 1;
}

int do_mget(int controlfd, char **argv, int mode) {
	char lbuf[NLSTBUF+1];
	char *name, *next = NULL;
	int n, l, filecount = 0;

	bzero(lbuf, NLSTBUF);
	argv++;			// move past command
	while (*argv) {
		if (do_nlst(controlfd, lbuf, NLSTBUF, *argv, mode) < 1) 
			return -1;
		if (!strlen(lbuf)) {
			printf("%s: not found\n", *argv++);
			continue;
		}
		name = lbuf;
		*(next = strchr(lbuf, '\r')) = '\0';
		while (name != NULL) {
			if ((l = ask("mget", name)) == -1) break;
			next += 2;
			if (!l) {
				if ((n = do_get(controlfd, name, NULL, mode)) < -1) 
					break;	// Error -2 is fatal, -1 = this file failed.
				if (n > 0) filecount++;
			}
			name = next;
			next = strchr(next, '\r');
			if (next == NULL) break;
			*next = '\0';
		}
		if (l == -1) break;
		argv++;
	}
	if (filecount > 1) printf("mget: %d files transferred.\n", filecount);
	return 1;
}

void settype(int controlfd, char t) {
	char b[CMDBUF];

	/* atype - active type (what we've told the server)
	 * type - selected file transfer type (via BIN or ASCII commands)
	 */
	if (atype == t) return;
	sprintf(b, "TYPE %c\r\n", t);
	send_cmd(controlfd, b);

	get_reply(controlfd, b, sizeof(b), 1);	/* Should be '200 Type set to ...' */
	if (*b == '2')
		atype = t;
	return;
}

int connect_cmd(char *ip, unsigned int server_port) {
	struct sockaddr_in servaddr;
	int controlfd;
	unsigned int myport;

	if ((controlfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Can't open socket - check if ktcp is running.\n");
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
	get_ip_port(controlfd, myip, &myport);
	if (debug > 2) printf("BIND: I am %s:%u\n", myip, myport);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(server_port);
	if (isalpha(*ip))
		servaddr.sin_addr.s_addr = in_gethostbyname(ip);	
	else
		servaddr.sin_addr.s_addr = in_aton(ip);	
	if (servaddr.sin_addr.s_addr == 0 ) {
		printf("Host name not found.\n");
		return -1;
	}
#ifdef QEMUHACK
	if (qemu) {
		long madr = in_aton(myip);
		long loopback = in_aton("127.0.0.1");
		if ((servaddr.sin_addr.s_addr == loopback) || (servaddr.sin_addr.s_addr == madr)) {
			printf("loopback detected, disabling QEMU mode.\n");
			qemu = 0;
			servaddr.sin_addr.s_addr = madr;
		}
	}
#endif

	if (debug > 1) printf("Connecting to %s @ port %u\n", in_ntoa(servaddr.sin_addr.s_addr), server_port);
	if (in_connect(controlfd, (struct sockaddr *) &servaddr, sizeof(servaddr), 10) < 0) {
		perror("ftp");
		controlfd = -1;
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
	//fclose(fcmd);
	close(controlfd);
	return;
}

int do_login(char *user, char *passwd, char *buffer, int len, char *ip, unsigned int port) {
	char localbf[50], *lip, *cp;
	int controlfd = -1;

#ifdef QEMUHACK
	// Check for QEMU mode - repeat on every login, since a loopback connection may have temporarily
	// disabled QEMU mode.
	if ((cp = getenv("QEMU")) != NULL) {
		qemu = atoi(cp);
		printf("QEMU set to %d\n", qemu);
		//if (qemu) debug++;      //FIXME: Temporary - for debugging
	}
#endif
	lip = ip;
	if (*lip == '\0') {	/* ask for server address */
				//FIXME: Needs sanity checking on input
		printf("Connect to: ");	
		lip = gets(localbf);
		if (!lip) return -1;
		ip = strtok(lip, " ");
		cp = strtok(NULL, " ");
		if (cp) port = atoi(cp);
		//printf("ip %s, port %u\n", ip, port);
	} 
	if ((controlfd = connect_cmd(ip, port)) < 0)
		return -1;
	//printf("Connected to %s.\n", ip);
	//fcmd = fdopen(controlfd, "r+");

	/* Get the ID message from the server */
	get_reply(controlfd, buffer, len, 0);	/* 220 XYZ ftp server ready etc. */

	/* Do the login process */

	printf("Name (%s:%s): ", ip, user);
	if (strlen(gets(localbf)) > 2) strcpy(user, localbf); /* no boundary checking */
	sprintf(buffer, "USER %s\r\n", user);
	send_cmd(controlfd, buffer);
	get_reply(controlfd, buffer, len, 1);	/* 331 Password required ... */
	if (*buffer != '3') {
		printf("Error in username: %s", buffer);
		//fclose(fcmd);
		return -1;
	}
	lip = getpass("Password:");
	printf("\n");
	if (strlen(lip) < 2) lip = passwd; /* use passwd from command line if present */
	sprintf(buffer, "PASS %s\r\n", lip);
	send_cmd(controlfd, buffer);
#if 0
	/* Gobble up welcome message - line by line to catch continuation */
	while (fgets(buffer, len, fcmd) != NULL) {
		printf("%s", buffer);
		if (buffer[3] == ' ') break;
	}
#else
	get_reply(controlfd, buffer, len, 1);
#endif
	if (*buffer != '2') {	/* 230 User xxx logged in. */
		printf("Login failed: %s", buffer);
		do_close(controlfd, buffer, len);
		return -1;
	}
	send_cmd(controlfd, "SYST\r\n");
	get_reply(controlfd, buffer, len, 1);
	if (strstr(buffer, "UNIX")) {
		printf("Remote system is Unix/Linux, using binary mode\n");
		type = BIN;
	} else printf("File transfer mode is ASCII\n");
	return controlfd;
}

void parm_err(void) {
	printf("Command needs parameter.\n");
}

int main(int argc, char **argv) {

	int server_port = DFLT_PORT, controlfd = -1, datafd, code;
	int mode = PASV, i, mput, mget, f_mode, autologin = 1;
	char command[BUF_SIZE], str[IOBUFLEN+1], user[30], passwd[30];
	char *param[MAXPARMS];

	strcpy(user, "ftp");
	*passwd = '\0';
	*srvr_ip = '\0';

	while (argc > 1) {
		argc--;
		argv++;
		if (**argv == '-') {
			switch ((*argv)[1]) {
			case 'u':
				strcpy(user, argv[1]);
				argv++; argc--;
				break;
			case 'p':
				strcpy(passwd, argv[1]);
				argv++; argc--;
				break;
			case 'd':
				if (isdigit(*argv[1]) && (strlen(argv[1]) == 1)) {
					debug = atoi(argv[1]);
					argv++; argc--;
				} else debug++;
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
				debug = 1;
				break;
			case 'i':
				prompt = FALSE;
				break;
#endif
#ifdef QEMUHACK
			case 'q':
				qemu++;
				break;
#endif
			default:
				printf("Error in arguments.\n");
				exit(-1);
			}
		} else {
			if ((strchr(*argv, '.') != 0) && (isdigit((int)**argv))) 
				strcpy(srvr_ip, *argv); 		//numeric ip
			else if (isalpha((int)**argv))
				strcpy(srvr_ip, in_ntoa(in_gethostbyname(*argv)));
			else 
				server_port = atoi(*argv);
		}
	}
	
	if (*srvr_ip == '\0') {
		autologin = 0;	// Don't attempt autologin unless there is a server name/address on the cmd line
	}
	if ((debug > 2) && (*srvr_ip != '\0')) 
		printf("cmd line: ip %s port %u %s\n", srvr_ip, server_port, (server_port == DFLT_PORT)? "(default)" : ""); 

	if (autologin) {
		if ((controlfd = do_login(user, passwd, command, sizeof(command), srvr_ip, server_port)) > 0)
			connected++;
		else server_port = DFLT_PORT;
	}
	bzero(command, BUF_SIZE);

	while(1) {
		mput = 0; mget = 0;
		f_mode = R_OK;
		if ((code = get_cmd(command, sizeof(command), param)) < 0) break; /* got EOF */
		if (check_connected) {
			connected = is_connected(controlfd);
			check_connected = 0;
		}

		// prequalify commands so we don't have to do mode checking on every command
		if (!connected && (code > CMD_NOCONNECT) && (code < CMD_CONNECT)) {
			printf("Not connected.\n");
			continue;
		}

		switch (code) {

		case CMD_OPEN:
			if (connected) {
				if (!(connected = is_connected(controlfd))) {
					printf("Server has gone away, connection closed\n");
				} else {
					printf("Already connected to %s, use close first.\n", srvr_ip);
					break;
				}
			}
			if (param[1]) {
				strcpy(srvr_ip, param[1]);
				server_port = DFLT_PORT;
				if (param[2])
					server_port = atoi(param[2]);
			}
			if ((controlfd = do_login(user, passwd, str, sizeof(str), srvr_ip, server_port)) < 0) 
				server_port = DFLT_PORT;	/* reset to default */
			else
				connected++;
			break;

		case CMD_CLOSE:
			do_close(controlfd, str, sizeof(str));
			connected = 0;
			server_port = DFLT_PORT;
			break;

		case CMD_MPUT:
			mput++;
			f_mode = X_OK;
			/* fall thru */

		case CMD_PUT:
			if (!param[1]) {
				//FIXME: Should ask for file names interactively 
				parm_err();
				break;
			}
#ifndef GLOB
			char name = param[1];
			if (*name == '*') *name = '.';  /* No globbing, full directory transfer */
			if (access(name, f_mode) < 0) {
				perror(name);
				break;
			}
#else
			code = f_mode; /* silence the compiler */
#endif
			if (mput)
				do_mput(controlfd, param, mode);
			else 
				do_put(controlfd, param[1], param[2], mode);
			break;


		case CMD_MGET:
			mget++;
			/* fall thru */

		case CMD_GET:
			if (!param[1]) {
				parm_err();
				break;
			}
			if (mget)
				do_mget(controlfd, param, mode);
			else 
				do_get(controlfd, param[1], param[2], mode);
			break;

		case CMD_DIR:
			if (type != ASCII) settype(controlfd, ASCII);
#ifdef BLOATED
			if (mode != PASV)
				datafd = do_active(controlfd);
			else
#endif
				datafd = do_passive(controlfd);
			if (datafd > 0) {
				if (do_ls(controlfd, datafd, param, mode) < 0) {
					check_connected++;
				}
			} else check_connected++;
			break;

		case CMD_SITE:		//FIXME: Add prompt for subcommand
			sprintf(str, "SITE %s %s\r\n", param[1]?param[1]:"", param[2]?param[2]:"");
			send_cmd(controlfd, str);
			get_reply(controlfd, command, sizeof(command), 0);
			break;
		
		case CMD_CWD:
			if (!param[1]) {
				parm_err();		// FIXME: No arg - return to home dir
				break;
			}
			sprintf(str, "CWD %s\r\n", param[1]);
			send_cmd(controlfd, str);
			get_reply(controlfd, command, sizeof(command), 0);
			break;
			
		case CMD_PWD:
			send_cmd(controlfd, "PWD\r\n");
			get_reply(controlfd, command, sizeof(command), 0);
			break;

		case CMD_QUIT:
			do_close(controlfd, command, sizeof(command));
			connected = 0;
			goto out;

#ifdef BLOATED
		case CMD_SHELL:
			if ((i = system(&command[1]))) 
				if (debug > 2) printf("Shell returned %d\n", i);	// the shell emits its own error msg
			break;
			
		case CMD_DELE:
			if (!param[1]) {
				parm_err();
				break;
			}
			sprintf(str, "DELE %s\r\n", param[1]);
			send_cmd(controlfd, str);
			get_reply(controlfd, command, sizeof(command), 0);
			break;

		case CMD_MKD:
			if (!param[1]) {
				parm_err();
				break;
			}
			sprintf(str, "MKD %s\r\n", param[1]);
			send_cmd(controlfd, str);
			get_reply(controlfd, command, sizeof(command), 0);
			break;

		case CMD_HELP:
			help();
			break;

		case CMD_LCWD:
			if (param[1]) {
				if (chdir(param[1]) < 0)
					perror(param[1]);
			}
			printf("Local directory: %s\n", getcwd(str, IOBUFLEN));
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
			connected = is_connected(controlfd);
			if (connected) {
				printf("Connected to %s [port %d] as user %s\n", srvr_ip, server_port, user);
				send_cmd(controlfd, "SYST\r\n");
				get_reply(controlfd, str, sizeof(str), 10);
				printf("Remote system: %s", &str[4]);
			}
			else 
				printf("Not connected.\n");
			if (!debug) 
				printf("Debug is off.\n");
			else 
				printf("Debug level: %d\n", debug);
			printf("Globbing: %s\n", glob ? "on" : "off");
			printf("Transfer mode: %s\n", (mode == PASV) ? "passive" : "active");
			printf("File mode: %s\n", (type == ASCII) ? "ASCII" : "IMAGE");
			printf("Multifile prompting is %s\n", prompt ? "on" : "off");
			// FIXME add statistics - file count, byte count etc.
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
			if (param[1]) {
				if (isdigit(*param[1]))
					debug = atoi(param[1]);
				else {
					if (!strcmp(param[1], "off")) debug = 0;
					else printf("Illegal parameter: %s\n", param[1]);
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
