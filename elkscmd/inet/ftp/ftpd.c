/*
 * Minimal FTP server for ELKS
 * November 2021 by Helge Skrivervik - helge@skrivervik.com
 *
 */

#include	<time.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<string.h>
#include	<arpa/inet.h>
#include	<unistd.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<netinet/in.h>
#include	<stdbool.h>
#include	<netdb.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<time.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include 	<dirent.h>

#define 	CMDBUFSIZ 	512
#define		IOBUFSIZ	1500
#define		TRUE		1
#define		FALSE		0
#define		FTP_PORT	21
#define		PASV_PORT	49821;	/* 'random' port for passive connections */

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

//static long myaddr;
static int debug = 0;

/* Trim leading and trailing whitespaces */
void trim(char *str) {
	int i;
	int begin = 0;
	int end = strlen(str) - 1;

	while (isspace((unsigned char) str[begin]))
        	begin++;

	while ((end >= begin) && (isspace((unsigned char) str[end])))
        	end--;

	// Shift all characters back to the start of the string array.
	for (i = begin; i <= end; i++)
        	str[i - begin] = str[i];

	str[i - begin] = '\0'; // Null terminate string.
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

int get_client_ip_port(char *str, char *client_ip, unsigned int *client_port){
	char *n1, *n2, *n3, *n4, *n5, *n6;
	int x5, x6;

	strtok(str, " ");
	n1 = strtok(NULL, ",");
	n2 = strtok(NULL, ",");
	n3 = strtok(NULL, ",");
	n4 = strtok(NULL, ",");
	n5 = strtok(NULL, ",");
	n6 = strtok(NULL, ",");

	sprintf(client_ip, "%s.%s.%s.%s", n1, n2, n3, n4);

	x5 = atoi(n5);
	x6 = atoi(n6);
	*client_port = (256*x5)+x6;

	if (debug) printf("client_ip: %s client_port: %u\n", client_ip, *client_port);
	return 1;
}

int setup_data_connection(char *client_ip, unsigned int client_port, int server_port){
	
	struct sockaddr_in cliaddr, tempaddr;
	int fd, sockwait = 0;

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
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

	/* bind port for data connection to be server port - 1 using a temporary struct sockaddr_in */
	bzero(&tempaddr, sizeof(tempaddr));
	tempaddr.sin_family = AF_INET;
	tempaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	tempaddr.sin_port = htons(server_port-1);

	while ((bind(fd, (struct sockaddr*) &tempaddr, sizeof(tempaddr))) < 0) {
		if (sockwait++ > 10 || errno != EADDRINUSE) {
			printf("Bind: Could not connect on port %d\n", server_port-1);
    			perror("bind error");
    			//tempaddr.sin_port   = htons(some_other_port);
			return -1;
		} 
		if (debug) printf(".");
		sleep(1);
		//server_port = 876; /* TESTING - 'random' (!!) #  below 1024 */
    		//tempaddr.sin_port   = htons(server_port - sockwait);
	}
	if (debug) printf("\n");

	//initiate data connection fd with client ip and client port             
	bzero(&cliaddr, sizeof(cliaddr));

	cliaddr.sin_family = AF_INET;
	cliaddr.sin_port = htons(client_port);
	cliaddr.sin_addr.s_addr = in_aton(client_ip);

	if (connect(fd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0) {
    		perror("connect error");
    		return -1;
	}

	return fd;
}

int get_filename(char *input, char *fileptr){

    char *filename = NULL;

	filename = strtok(input, " ");
	filename = strtok(NULL, " \r\n");
	//printf("get_filename: filename=<%s>\n", filename);
	
	if (filename == NULL) {
        	return -1;
	} else {
    		strncpy(fileptr, filename, strlen(filename));
    		return 1;
    }
}

int get_command(char *command){

	int value = 0;

	// FIXME - there are far more elegant ways of doing this!
	if (strncmp(command, "LIST", 4) == 0)	  {value = CMD_LIST;}
	else if (strncmp(command, "RETR", 4) == 0) {value = CMD_RETR;}
	else if (strncmp(command, "STOR", 4) == 0) {value = CMD_STOR;}
	else if (strncmp(command, "ABOR", 4) == 0) {value = CMD_ABOR;}
	else if (strncmp(command, "SYST", 4) == 0) {value = CMD_SYST;}
	else if (strncmp(command, "QUIT", 4) == 0) {value = CMD_QUIT;}
	else if (strncmp(command, "PASV", 4) == 0) {value = CMD_PASV;}
	else if (strncmp(command, "PORT", 4) == 0) {value = CMD_PORT;}
	else if (strncmp(command, "TYPE", 4) == 0) {value = CMD_TYPE;}
	else if (strncmp(command, "DELE", 4) == 0) {value = CMD_DELE;}
	else if (strncmp(command, "NLST", 4) == 0) {value = CMD_NLST;}
	else if (strncmp(command, "PWD", 3) == 0) {value = CMD_PWD;}
	else if (strncmp(command, "CWD", 3) == 0) {value = CMD_CWD;}
	else if (strncmp(command, "MKD", 3) == 0) {value = CMD_MKD;}
	else value = CMD_UNKNOWN;
	return value;
}

/* For now - 'fake' login processing - always returns OK */
/* If/when LOGIN is changed to do real user authentication, */
/* move the cmd processing to the main command loop, add LOGOUT */
/* support and use setuid()/seteuid() in the child process. */

int do_login(int controlfd) {
	char *str = "331 User OK.\r\n";
	char buf[200];

	if(read(controlfd, buf, 200) < 0)
		return -1;
	if (strncmp(buf, "USER", 4)) 
		return -1;
	write(controlfd, str, strlen(str));

	if(read(controlfd, buf, 200) < 0)
		return -1;
	if (strncmp(buf, "PASS", 4)) 
		return -1;
	str = "230 Password OK.\r\n";
	chdir("/root");		/* brute force ... */
	write(controlfd, str, strlen(str));
	return 0; /* always OK */
}

int do_list(int controlfd, int datafd, char *input) {
	char *str, cmd_buf[CMDBUFSIZ], iobuf[IOBUFSIZ];
   	FILE *in;
	DIR *dir;
	struct dirent *dp;
	int len, nlst = 0;;

	bzero(cmd_buf, sizeof(cmd_buf));
	bzero(iobuf, sizeof(iobuf));

	str = "150 Opening ASCII mode data connection for /bin/ls.\r\n";
	if (strncmp(input, "NLST", 4) == 0) { /* this is for MGET, should handle globbing */
		nlst++;
		str = "125 List started OK.\r\n";
	}
	if (get_filename(input, iobuf) > 0) {
		trim(iobuf);
		sprintf(cmd_buf, "/bin/ls -l %s", iobuf);
	} else {
		*iobuf = '.';	
		strcat(cmd_buf, "/bin/ls -l");
	}

	if (debug) printf("LIST/NDIR: '%s' fd %d\n", cmd_buf, datafd);
    	dir = opendir(iobuf); 	/* test for existence */
    	if (!dir) {
    		str= "550 No Such File or Directory.\r\n";
    		write(controlfd, str, strlen(str));
    		return -1;
    	} 
    	write(controlfd, str, strlen(str));

	/* NLST processing, just a simple list of names */
	if (nlst) {
		len = 0;
		*iobuf = '\0';
		while ((dp = readdir(dir)) != NULL) {
			if (*dp->d_name != '.') {
				//printf("%s\n", dp->d_name);
				if (len + dp->d_namlen + 2 < IOBUFSIZ) {
					strcat(iobuf, dp->d_name);
					strcat(iobuf, "\r\n");
					len += dp->d_namlen +2;
				} else {
					write(datafd, iobuf, strlen(iobuf));
					len = 0;
					bzero(iobuf, sizeof(iobuf));
				}
			}
		}
		if (len) {
			write(datafd, iobuf, strlen(iobuf));
			//printf("NLST <%s>\n", iobuf);
		}
		closedir(dir); 
		return 2;
	}
	closedir(dir); 

	// LIST - pipe output from ls-command back to the client

	if (!(in = popen(cmd_buf, "r"))) {
    		str = "451 Requested action aborted. Local error in processing.\n";
    		write(controlfd, str, strlen(str));
        	return -1;
	}
	bzero(iobuf, IOBUFSIZ);
//#define BUFFERED_DIR
#ifndef BUFFERED_DIR
	int status = 1;
	while (fgets(iobuf, IOBUFSIZ-1, in) != NULL) {
		len = strlen(iobuf);
		iobuf[len-1] = '\r';    /* Fix FTP ASCII-style line endings */
		iobuf[len++] = '\n';
		if (write(datafd, iobuf, len) != len) {
			printf("LIST: fd %d len%d\n", datafd, len);
			perror("LIST: Data socket write error");
			status = -1;
			break;
		}
		bzero(iobuf, sizeof(iobuf));
#else
	int bufpos = 0, status = 1;
	while (fgets(cmd_buf, CMDBUFSIZ-1, in) != NULL) {
		len = strlen(cmd_buf);
		cmd_buf[len-1] = '\r';	/* Fix FTP ASCII-style line endings */
		cmd_buf[len] = '\n';
		cmd_buf[++len] = '\0';
		//printf("%s", cmd_buf);
		if ((len + bufpos) < IOBUFSIZ) {
			strcat(iobuf,cmd_buf);
			bufpos += len;
		} else {
			status = write(datafd, iobuf, bufpos);
			printf("%d:", status);
			bzero(iobuf, sizeof(iobuf));
			bufpos = 0;
			if (status < 0) {perror("Write socket"); break;}
		}
	}
	if (bufpos) {
		status = write(datafd, iobuf, bufpos);
		printf("%d\n", status);
		if (status < 0) perror("Write socket"); 
#endif
	}
	close(datafd);
	pclose(in);

	return status;
}

/* Passive mode: Server listens for incoming data connection */
int do_pasv(int controlfd, int *datafd) {
	int fd, i = 1, port = PASV_PORT;
	struct sockaddr_in pasv;
	char *p, *a;
	char str[100], *pasv_err = "425 Can't open passive connection.\r\n";

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    		write(controlfd, pasv_err, strlen(pasv_err));
		perror("PASV: Socket open failed");
		return -1;
	}

	//if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0) 
		//perror("SO_REUSEADDR");
	i = SO_LISTEN_BUFSIZ;
	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &i, sizeof(i)) < 0)
                perror("SO_RCVBUF");

	//bzero(&pasv, sizeof(pasv));
	pasv.sin_family = AF_INET;
	//pasv.sin_addr.s_addr = myaddr;
	pasv.sin_addr.s_addr = htonl(INADDR_ANY);
	pasv.sin_port = htons(port);
	i = 0;
	while (bind(fd, (struct sockaddr *)&pasv, sizeof(pasv)) < 0) {
		if (debug) printf("PASV bind failed: host %s port %u\n", in_ntoa(pasv.sin_addr.s_addr), ntohs(pasv.sin_port));
		perror("PASV bind failure");
		if (i++ > 10 || errno != EADDRINUSE) {
			if (debug) printf("Bind: Could not connect on port %u\n", port);
    			perror("bind error");
    			write(controlfd, pasv_err, strlen(pasv_err));
			return -1;
		} 
		port++;
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
		if (debug) printf("PASV: Listen failed.\n");
		close(fd);
		return -1;
	}
	a = (char *) &pasv.sin_addr;
	//a = (char *) &myaddr;
	p = (char *) &pasv.sin_port;
	sprintf(str, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
		UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
    	write(controlfd, str, strlen(str));
	//if (debug) printf("%s", str);
	i = sizeof(pasv);
	if ((*datafd = accept(fd, (struct sockaddr *)&pasv, (unsigned int *)&i)) < 0 ) {
		perror("accept error");	
		close(fd);
		return -1;
	}
	close(fd);
	printf("Accepted connection from %s/%u on fd %d\n", in_ntoa(pasv.sin_addr.s_addr), ntohs(pasv.sin_port), i);
	//*datafd = i;

	return 0;

}


int do_retr(int controlfd, int datafd, char *input){
	char *str, cmd_buf[CMDBUFSIZ], iobuf[IOBUFSIZ];
	int fd, len;
	struct stat fst;

	bzero(cmd_buf, sizeof(cmd_buf));
	bzero(iobuf, sizeof(iobuf));

	/* FIXME - verify that the data connection is actually open */
	if (get_filename(input, cmd_buf) > 0) {
		trim(cmd_buf);
		if (debug) printf("RETR: <%s> fd %d\n", cmd_buf, datafd);
		if ((fd = open(cmd_buf, O_RDONLY)) > 0 ) {
			fstat(fd, &fst);
			sprintf(iobuf, "150 Opening BINARY data connection for %s (%ld bytes).\r\n", cmd_buf, fst.st_size);
			write(controlfd, iobuf, strlen(iobuf));
			while ((len = read(fd, iobuf, sizeof(iobuf))) > 0) 
				if (write(datafd, iobuf, len) != len) {
					printf("RETR error fd %d len %d\n", datafd, len);
					perror("Data write error"); 
					break;
				}
			close(fd);
		} else {
			str = "550 No Such File or Directory.\r\n";
    			write(controlfd, str, strlen(str));
    			return -1;
		}
	} else {  /* FIXME: this part may not be required */
		if (debug) printf("File not found: %s\n", cmd_buf);
		str = "450 Requested file action not taken.\r\nFilename Not Detected.\r\n";
    		write(controlfd, str, strlen(str));
		return -1;
	}
	return 1;
}

int do_stor(int controlfd, int datafd, char *input) {
	char *str, cmd_buf[CMDBUFSIZ], iobuf[IOBUFSIZ];
	int fp;
	int n = 0, len;

	bzero(cmd_buf, sizeof(cmd_buf));
	bzero(iobuf, sizeof(iobuf));


	if (get_filename(input, cmd_buf) < 0) {
		if (debug) printf("No file specified.\n");
		str = "450 Requested file action not taken.\r\n";
    		write(controlfd, str, strlen(str));
		return -1;
	}

	/* FIXME - verify that the data connection is actually open */
	/* FIXME - need to query protection mode for the source file and use that */
	//trim(cmd_buf);
	if ((fp = open(cmd_buf, O_CREAT|O_RDWR, 0644)) < 1) {
		perror("File create failure");
		str = "552 File create error, transfer aborted.\r\n";
		write(controlfd, str, strlen(str));
		return -1;
	}

	sprintf(iobuf, "150 Opening BINARY data connection for '%s'.\r\n", cmd_buf);
	write(controlfd, iobuf, strlen(iobuf));
	while ((n = read(datafd, iobuf, sizeof(iobuf))) > 0) {
		if ((len = write(fp, iobuf, n)) != n) {
			if (len < 0 )
				perror("File write error");
			else
				printf("File write error: %s\n", cmd_buf);
			close(fp);
			str = "552 Storage space exceeded, transfer aborted.\r\n";
			write(controlfd, str, strlen(str));
			return -2;
		}
	}

	close(fp);
	return 1;
}

void usage() {
	printf("Usage: ftpd [-d] [<listen-port>]\n");
}


int main(int argc, char **argv){

	int listenfd, connfd, ret, port = FTP_PORT;
	//int passive_mode = 0;
	struct sockaddr_in servaddr;
	pid_t pid;

	if (argc > 2) {
		usage();
		exit(1);
	}

	//myaddr = in_gethostbyname("elks");
	while (--argc) {
		argv++;
		if (*argv[0] == '-') 
			if (argv[0][1] == 'd')
				debug ++;
			else
				usage(), exit(-1);
		else 
			port = atoi(argv[0]);
	}
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket error");
		exit(1);
	}

#if 0
	/* set local port reuse, allows server to be restarted in less than 10 secs */
	ret = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret)) < 0)
		perror("SO_REUSEADDR");
#endif

	/* set small listen buffer to save ktcp memory */
	ret = SO_LISTEN_BUFSIZ;
	if (setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &ret, sizeof(int)) < 0)
		perror("SO_RCVBUF");

	//bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port	= htons(port);
	
	if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0 ) {
		perror("Cannot bind socket");
		exit(2);
	}

	if (listen(listenfd, 3) < 0 ) {
		perror("Error in listen");
		exit(3);
	}
	if (!debug) {
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
	}
	while (1) {
		if ((connfd = accept(listenfd, NULL, NULL)) < 0) {
			perror("Accept error:");
			break;
		}
		if (debug) printf("New Client Detected...\n");
		/* child process */
		if((pid = fork()) == 0) {
			close(listenfd);

			int datafd = -1, code, quit = FALSE;
			unsigned int client_port = 0;
			//char type = 'I';
			char client_ip[50], command[256], namebuf[50];
			char *str = "220 Welcome - ELKS minimal FTP server speaking.\n";
			char *complete = "226 Transfer Complete.\r\n";

			if (debug) printf("Child running - cmd chan is %d.\n", connfd);
			write(connfd, str, strlen(str));
			/* Do the standard housekeeping */
			/* FIXME: this needs to go into the main command loop */
			if (do_login(connfd) < 0) {
				printf("Login failed, terminating session.\n");
				break;
			}

			/* Main command loop */
			while(1) {
				bzero(command, (int)sizeof(command));
				if (read(connfd, command, sizeof(command)) <= 0) 
					break;
				clean(command);
    				code = get_command(command);
    				if (debug) printf("cmd: '%s' %d\n", command, code);

				switch (code) {

				case CMD_PORT:
					if (datafd >= 0) { /* connection already open, close it! */
						close(datafd);
						datafd = -1;
					}
    					get_client_ip_port(command, client_ip, &client_port);
    					if ((datafd = setup_data_connection(client_ip, client_port, port)) < 0) {
						str = "425 Can't open data connection.\r\n";
						if (debug) printf("PORT command failed.\n");
						datafd = -1;
						write(connfd, str, strlen(str));
					} else {
						str = "200 PORT command successful.\r\n";
						write(connfd, str, strlen(str));
					}
					break;

				case CMD_PASV: /* Enter Passive mode */
					if (datafd >= 0) { /* connection already open, close it! */
						close(datafd);
						datafd = -1;
					}
					if (do_pasv(connfd, &datafd) < 0) {
						str = "502 PASV: Cannot open server socket.\r\n";
						write(connfd, str, strlen(str));
						close(datafd);
						datafd = -1;
					}
					break;

				case CMD_NLST:
    				case CMD_LIST:	/* List files */
					if (datafd >= 0) {
    						if (do_list(connfd, datafd, command) < 2)
		    					write(connfd, complete, strlen(complete));
						else {
							/* NLST - different resonse codes */
							str = "250 List completed successfully.\r\n";
		    					write(connfd, str, strlen(str));
						}
					} else {
						str = "503 Bad sequence of commands.\r\n";
						write(connfd, str, strlen(str));
					}
					datafd = -1;
					break;

    				case CMD_RETR: /* Retrieve files */
					if (datafd < 0) { /* no data connection, don't even try ... */
						str = "426 Connection closed, transfer aborted.\r\n";
		    				write(connfd, str, strlen(str));
						break;
					}
    					if (do_retr(connfd, datafd, command) > 0 ) 
		    				write(connfd, complete, strlen(complete));
					/* if there was an error, the error reply has already been sent, 
					 * this does not make sense */
					close(datafd);
					datafd = -1;
					break;

    				case CMD_STOR: /* Store files */
					if (datafd < 0) { /* no data connection, don't even try ... */
						str = "426 Connection closed, transfer aborted.\r\n";
		    				write(connfd, str, strlen(str));
						break;
					}
    					if ((code = do_stor(connfd, datafd, command)) > 0) {
		    				write(connfd, complete, strlen(complete));
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
					break;

				case CMD_SYST: /* SYST command */
		    			str = "215 UNIX Type: L8 (Linux)\r\n";
		    			write(connfd, str, strlen(str));
					break;

				case CMD_QUIT: /* QUIT, terminate loop */
					quit = TRUE;
		    			if (debug) printf("Quitting...\n");
		    			str = "221 Goodbye.\n";
		    			write(connfd, str, strlen(str));
					if (datafd < 0) close(datafd);
		    			close(connfd);
					break;

				case CMD_TYPE: 	/* ASCII or binary, ignored for now */
					str = strtok(command, " ");
					str = strtok(NULL, " ");
					//type = *str;
					str = "200 Command OK.\r\n";
		    			write(connfd, str, strlen(str));
					break;

				case CMD_DELE:
					bzero(namebuf, sizeof(namebuf));
					if (get_filename(command, namebuf) < 0) {
						str = "501 Syntax error - DELE needs parameter.\r\n";
		    				write(connfd, str, strlen(str));
					} else {
						trim(namebuf);
						if (unlink(namebuf) < 0) {
							if (debug) printf("Cannot delete %s\n", namebuf);
							str = "550 No action - file not found.\r\n";
		    					write(connfd, str, strlen(str));
						} else {
							str = "250 Delete successful.\r\n";
		    					write(connfd, str, strlen(str));
						}
					}
					break;

				case CMD_PWD:
					str = getcwd(client_ip, 50); /* use client_ip as temp buffer */
					sprintf(command, "257 \"%s\" is current directory.\r\n", client_ip);
		    			write(connfd, command, strlen(command));
					break;

				case CMD_CWD:
					/* FIXME: if no arg, change back to home dir */
					/* namebuf is only 50 bytes, check for overrun */
					bzero(namebuf, sizeof(namebuf));
					if (get_filename(command, namebuf) < 0) {
						str = "501 Syntax error - CWD needs parameter.\r\n";
		    				write(connfd, str, strlen(str));
					} else {
						trim(namebuf);
						if (chdir(namebuf) < 0) {
							if (debug) printf("Cannot change directory to %s\n", namebuf);
							str = "550 No action - directory not found.\r\n";
		    					write(connfd, str, strlen(str));
						} else {
							str = "250 CWD successful.\r\n";
		    					write(connfd, str, strlen(str));
						}
					}
					break;

				case CMD_UNKNOWN:
					str = "503 Unknown command \r\n";
		    			write(connfd, str, strlen(str));
					break;
				}
				if (quit == TRUE) break;

			}
    			if (debug) printf("Child process exiting...\n");
    			close(connfd);
    			_exit(1);
		}
		/* End child process */

		close(connfd);
	}
}
