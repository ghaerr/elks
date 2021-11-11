
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

#define 	MAXLINE 	1024
#define		LISTENQ		1024
#define		TRUE		1
#define		FALSE		0
#define		FTP_PORT	21

/* Commands */
#define		CMD_LIST	1
#define		CMD_RETR	2
#define		CMD_STOR	3
#define		CMD_SYST	4
#define		CMD_PASV	5
#define		CMD_PORT	6
#define		CMD_PASS	7
#define		CMD_USER	8
#define		CMD_CLOS	9
#define		CMD_ABOR	10
#define		CMD_QUIT	11
#define		CMD_SKIP	12
#define		CMD_TYPE	13
#define		CMD_PWD		14
#define		CMD_DELE	15
#define		CMD_CWD		16
#define		CMD_MKD		17
#define		CMD_NLST	18
#define		CMD_UNKNOWN	30

static int debug = 0;

//function trims leading and trailing whitespaces
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
	int fd, sockwait = 0, on = 1;

	if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    		perror("socket error");
    		return -1;
	}
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof (on)) < 0) {
		/* This should not happen */
		printf("Data channel socket error: REUSEADDR, closing channel.\n");
		close(fd);
		return -1;
	}

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
	cliaddr.sin_addr.s_addr = in_gethostbyname(client_ip);

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
	else if (strncmp(command, "SKIP", 4) == 0) {value = CMD_SKIP;}
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
	/* FIXME: Add unknown command as default */
	return value;
}

/* For now - 'fake' login processing - always returns OK */
/* If/when LOGIN is changed to do real user authentication, */
/* move the cmd processing to the main command loop, add LOGOUT */
/* support and use setuid()/seteuid() in the child process. */

int do_login(int controlfd, char *buf) {
	char *str = "331 User OK.\r\n";

	//bzero(buf, MAXLINE);
    	if(read(controlfd, buf, MAXLINE) < 0)
		return -1;
	if (strncmp(buf, "USER", 4) < 0) 
		return -1;
	write(controlfd, str, strlen(str));

	//bzero(buf, MAXLINE);
    	if(read(controlfd, buf, MAXLINE) < 0) 
		return -1;
	if (strncmp(buf, "PASS", 4) < 0) 
		return -1;
	str = "230 Password OK.\n";
	write(controlfd, str, strlen(str));
	return 0; /* always OK */
}

int do_list(int controlfd, int datafd, char *input){
	char *str, cmd_buf[MAXLINE], iobuf[MAXLINE];
   	FILE *in;
	DIR *dir;
	struct dirent *dp;
	int len, nlst = 0;;
    	extern FILE *popen();
	extern int pclose();

	bzero(cmd_buf, (int)sizeof(cmd_buf));
	bzero(iobuf, (int)sizeof(iobuf));

	str = "150 Opening ASCII mode data connection for /bin/ls.\r\n";
	if (strncmp(input, "NLST", 4) == 0) { /* this is for MGET, must handle globbing
					       * not working yet */
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

	if (debug) printf("LIST/NDIR: '%s'\n", cmd_buf);
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
				if (len + dp->d_namlen + 2 < MAXLINE) {
					strcat(iobuf, dp->d_name);
					strcat(iobuf, "\r\n");
					len += dp->d_namlen +2;
				} else {
					write(datafd, iobuf, strlen(iobuf));
					len = 0;
					bzero(iobuf, (int)sizeof(iobuf));
				}
			}
		}
		if (len) {
			write(datafd, iobuf, strlen(iobuf));
			printf("NLST <%s>\n", iobuf);
		}
		closedir(dir); 
		return 2;
	}
	closedir(dir); 

	// LIST - create pipe for ls-command

	if (!(in = popen(cmd_buf, "r"))) {
    		str = "451 Requested action aborted. Local error in processing.\n";
    		write(controlfd, str, strlen(str));
        	return -1;
	}

	while (fgets(iobuf, MAXLINE-1, in) != NULL) {
		len = strlen(iobuf);
		iobuf[len-1] = '\r';	/* Fix FTP ASCII-style line endings */
		iobuf[len] = '\n';
		write(datafd, iobuf, len+1);
		bzero(iobuf, (int)sizeof(iobuf));
	}
	pclose(in);

	return 1;
}

int do_retr(int controlfd, int datafd, char *input){
	char *str, cmd_buf[MAXLINE], iobuf[MAXLINE];
	int fd, len;
	struct stat fst;

	bzero(cmd_buf, (int)sizeof(cmd_buf));
	bzero(iobuf, (int)sizeof(iobuf));

	/* FIXME - verify that the data connection is actually open */
	if (get_filename(input, cmd_buf) > 0) {
		trim(cmd_buf);
		if (debug) printf("RETR: <%s>\n", cmd_buf);
		if ((fd = open(cmd_buf, O_RDONLY)) > 0 ) {
			fstat(fd, &fst);
			sprintf(iobuf, "150 Opening BINARY data connection for %s (%ld bytes).\r\n", cmd_buf, fst.st_size);
			write(controlfd, iobuf, strlen(iobuf));
			while ((len = read(fd, iobuf, sizeof(iobuf))) > 0)
				write(datafd, iobuf, len);
			close(fd);
		} else {
			str = "550 No Such File or Directory\n";
    			write(controlfd, str, strlen(str));
    			return -1;
		}
	} else {  /* FIXME: this part may not be required */
		if (debug) printf("File not found: %s\n", cmd_buf);
		str = "450 Requested file action not taken.\nFilename Not Detected\n";
    		write(controlfd, str, strlen(str));
		return -1;
	}
	return 1;
}

int do_stor(int controlfd, int datafd, char *input) {
	char *str, cmd_buf[MAXLINE], iobuf[MAXLINE];
	int fp;
	int n = 0, len;

	bzero(cmd_buf, (int)sizeof(cmd_buf));
	bzero(iobuf, (int)sizeof(iobuf));


	if (get_filename(input, cmd_buf) < 0) {
		if (debug) printf("No file specified.\n");
		str = "450 Requested file action not taken.\n";
    		write(controlfd, str, strlen(str));
		return -1;
	}

	/* FIXME - verify that the data connection is actually open */
	/* FIXME - need to query protection mode for the source file and use that */
	//trim(cmd_buf);
	if ((fp = open(cmd_buf, O_CREAT|O_RDWR, 0644)) < 1) {
        	perror("file error");
        	return -1;
	}

	sprintf(iobuf, "150 Opening BINARY data connection for '%s'.\r\n", cmd_buf);
	write(controlfd, iobuf, strlen(iobuf));
    	while ((n = read(datafd, iobuf, MAXLINE)) > 0) {
		if ((len = write(fp, iobuf, n)) != n) {
			if (len < 0 )
				perror("File write error");
			else
				printf("File write error: %s\n", cmd_buf);
			close(fp);
			str = "552 Storage space exceeded, transfer aborted.\r\n";
			write(controlfd, str, strlen(str));
			return -1;
		}
	}

	close(fp);
	return 1;
}

void usage() {
	printf("Usage: ftpd [-d] [<listen-port>]\n");
}

int main(int argc, char **argv){

	int listenfd, connfd, port = FTP_PORT;
	struct sockaddr_in servaddr;
	pid_t pid;

	if (argc > 2) {
		usage();
		exit(1);
	}

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
		perror("socket error:");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port);
	
	if (bind(listenfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0 ) {
		perror("Cannot bind socket");
		exit(2);
	}

	if (listen(listenfd, LISTENQ) < 0 ) {
		perror("Error in listen");
		exit(3);
	}

	while(1){
		if ((connfd = accept(listenfd, NULL, NULL)) < 0) {
			perror("Accept error:");
			break;
		}
		if (debug) printf("New Client Detected...\n");
		//child process---------------------------------------------------------------
		if((pid = fork()) == 0) {
			close(listenfd);

			int datafd = 0, code, quit = FALSE;
			unsigned int client_port = 0;
			char type = 'I';
			char client_ip[50], command[MAXLINE], namebuf[50];
			char *str = "220 Welcome - ELKS minimal FTP server speaking.\n";
			char *complete = "226 Transfer Complete.\r\n";

			if (debug) printf("Child running.\n");
			write(connfd, str, strlen(str));
			/* Do the standard housekeeping */
			/* FIXME: this needs to go into the main command loop */
			if (do_login(connfd, command) < 0) {
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
					if (datafd > 0) { /* connection already open, close it! */
						close(datafd);
						datafd = 0;
					}
    					get_client_ip_port(command, client_ip, &client_port);
    					if ((datafd = setup_data_connection(client_ip, client_port, port)) < 0) {
						str = "425 Can't open data connection.\r\n";
						if (debug) printf("PORT command failed.\n");
						datafd = 0;
						write(connfd, str, strlen(str));
					} else {
						str = "200 PORT command successful.\r\n";
						write(connfd, str, strlen(str));
					}
					break;

				case CMD_NLST:
    				case CMD_LIST:	/* List files */
					if (datafd > 0) {
    						if (do_list(connfd, datafd, command) < 2)
                    					write(connfd, complete, strlen(complete));
						else {
							/* NLST - different resonse codes */
							str = "250 List completed successfully.\r\n";
                    					write(connfd, str, strlen(str));
						}
						close(datafd);
					} else {
						str = "503 Bad sequence of commands.\r\n";
						write(connfd, str, strlen(str));
					}
					datafd = 0;
					break;

    				case CMD_RETR: /* Retrieve files */
    					//get_client_ip_port(recvline, client_ip, &client_port);
    					//if ((datafd = setup_data_connection(client_ip, client_port, port)) < 0)
    					if (do_retr(connfd, datafd, command) > 0 ) 
                    				write(connfd, complete, strlen(complete));
					/* if there was an error, the error reply has already been sent, 
					 * this does not make sense */
					close(datafd);
					datafd = 0;
					break;

    				case CMD_STOR: /* Store files */
    					if (do_stor(connfd, datafd, command) > 0) {
                    				write(connfd, complete, strlen(complete));
						close(datafd);
					} else 
						net_close(datafd, 1); /* Error: Need RST to stop data flow */
					datafd = 0;
					break;

    				case CMD_SKIP: /* SKIP - FIXME */
                    			str = "550 Filename Does Not Exist.\r\n";
                    			write(connfd, str, strlen(str));
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
                    			close(connfd);
					break;

				case CMD_TYPE: 	/* ASCII or binary, ignored for now */
					str = strtok(command, " ");
					str = strtok(NULL, " ");
					type = *str;
					str = "200 Command OK.\r\n";
                    			write(connfd, str, strlen(str));
					break;

				case CMD_PWD:
					/* FIXME: Get real current directory */
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

				case CMD_DELE:
				case CMD_PASV: /* Enter Passive mode */
					str = "502 Not Implemented.\r\n";
                    			write(connfd, str, strlen(str));
					break;
				}
				if (quit == TRUE) break;

			}
    			if (debug) printf("Exiting Child Process...\n");
    			close(connfd);
    			_exit(1);
		}
		//end child process-------------------------------------------------------------
		close(connfd);
	}
}
