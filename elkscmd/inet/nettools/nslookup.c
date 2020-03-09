
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>

#ifndef __linux__
#include <linuxmt/un.h>
#include <linuxmt/in.h>
#include "linuxmt/arpa/inet.h"
#else
#include <sys/types.h>
#include <netdb.h>
#endif

struct DNS_HEADER
{
  __u16 len;
  __u16 id; 	   /* identification number */
  __u16 flags;
  __u16 q_count;   /* number of questions */
  __u16 ans_count; /* number of answer RRs */
  __u16 auth_count;/* number of authority RRs */
  __u16 add_count; /* number of additional RRs */
};

struct QUESTION
{
    __u16 qtype;
    __u16 qclass;
};

unsigned long int in_aton(const char *str)
{
    unsigned long l = 0;
    unsigned int val;
    int i;

    for (i = 0; i < 4; i++) {
	l <<= 8;
	if (*str != '\0') {
	    val = 0;
	    while (*str != '\0' && *str != '.') {
		val *= 10;
		val += *str++ - '0';
	    }
	    l |= val;
	    if (*str != '\0')
		str++;
	}
    }
    return htonl(l);
}

/* convert e.g. www.google.com (host) to 3www6google3com (dns) */
void ChangetoDnsNameFormat(unsigned char* dns,unsigned char* host)
{
    int lock = 0 , i;
    strcat((char*)host,".");

    for (i = 0 ; i < strlen((char*)host) ; i++)
    {
        if (host[i] == '.')
        {
            *dns++ = i-lock;
            for (;lock<i;lock++)
            {
                *dns++=host[lock];
            }
            lock++; /*or lock=i+1; */
        }
    }
    *dns++='\0';
}

/* Get the DNS server from the /etc/resolv.conf file */
char* get_dns_server()
{
    FILE *fp;
    static char line[200];
    char ipaddress[200];
    char* ptr;
    if ((fp = fopen("/etc/resolv.conf" , "r")) == NULL)
    {
        //printf("failed to read resolv.conf\n");
	sprintf(line,"208.67.222.222"); /*nameserver*/
	return &line;
    }

    while (fgets(line , 200 , fp))
    {
        if (line[0] == '#')
        {
            continue;
        }
        if (strncmp(line , "nameserver" , 10) == 0)
        {
	    sprintf(ipaddress,"%s",line); /*strtok modifies the string, use copy*/
            ptr = strtok(ipaddress, " "); /*first part = "nameserver"*/
            ptr = strtok(NULL , " "); /*second part = ip address */
	    sprintf(line,"%s",ptr);
	    line[strlen(line)-1]='\0'; /*strtok adds trailing blank*/
	    if (strlen(ipaddress)<8) {
	      //printf("ip adress not read\n");
	      sprintf(line,"208.67.222.222"); /*default nameserver*/
	    }
	    fclose (fp);
	    return &line;
        }
    }

}


int main(int argc, char *argv[]) {
  static char outbuf[200];
  char inbuf[200];
  char hostname[200];
  const char* nameserver;
  unsigned char *qname;
  int query_type;
  int fd,rc;
  struct sockaddr_in addr;
  struct DNS_HEADER *dns = NULL;
  struct QUESTION *qinfo = NULL;

  printf("Please enter hostname to lookup : ");
  scanf("%s" , hostname);

  if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }
  memset(&addr, 0, sizeof(addr));

  addr.sin_family = AF_INET;
  addr.sin_port = 0; /* any port */
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
        perror("Bind failed");
	exit(1);
  }

  addr.sin_family = AF_INET;
  nameserver=get_dns_server();
  printf("Nameserver queried:%s\n",nameserver);
  addr.sin_addr.s_addr = in_aton(nameserver);
  addr.sin_port = htons(53);

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("connect error");
    close(fd);
    exit(-1);
  }

  dns = (struct DNS_HEADER *)&outbuf;
  /*dns->len:  set at end */
  dns->id = (__u16) htons(getpid());
  dns->flags = 0x01; /*This is a standard query */
  dns->q_count = htons(1); /*we have only 1 question */
  dns->ans_count = 0;
  dns->auth_count = 0;
  dns->add_count = 0;

  qname =(unsigned char*)&outbuf[sizeof(struct DNS_HEADER)];   /*point to the query portion */

  ChangetoDnsNameFormat(qname , hostname);

  qinfo =(struct QUESTION*)&outbuf[sizeof(struct DNS_HEADER) + (strlen((const char*)qname) + 1)]; /*fill it */
  query_type=1; /*IPv4 address */
  qinfo->qtype = htons(query_type);
  qinfo->qclass = htons(1); /* internet */

  dns->len = (sizeof(struct DNS_HEADER) + strlen((const char*)qname) + sizeof(struct QUESTION) -1) * 0x0100;

  rc = write(fd, outbuf, dns->len+1);

  if (rc < 0) {
        perror("write error");
	close(fd);
        exit(-1);
    }

  bzero( inbuf, sizeof(inbuf));
  rc=read(fd,inbuf,200);
  //printf("\n%d bytes read\n",rc);
  //for (i=0;i<50;i++) printf("%2X,",inbuf[i]);
  printf("%s has the ip address: ",hostname);
  printf("%d.%d.%d.%d\n",inbuf[rc-4],inbuf[rc-3],inbuf[rc-2],inbuf[rc-1]);

  close(fd);
  return 0;
}

