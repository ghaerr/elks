/*
 *  fdisk.c  Disk partitioning program.
 *  Copyright (C) 1997  David Murn
 *
 *  This program is distributed under the GNU General Public Licence, and
 *  as such, may be freely distributed.
 *
 */

#ifdef linux
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/hdreg.h>
#else
#include <stdio.h>
#include <linuxmt/types.h>
#include <linuxmt/errno.h>
#include <linuxmt/genhd.h>
#include <linuxmt/hdreg.h>
#include <linuxmt/major.h>
#include <linuxmt/bioshd.h>
#include <linuxmt/fs.h>
#include <linuxmt/string.h>
#include <linuxmt/fcntl.h>
#endif
#include "fdisk.h"

#define die { printf("Usage %s [-l] <device-name>\n",argv[0]); fflush(stdout); exit(1); }
#define spc ((unsigned long)(geometry.heads*geometry.sectors))

static Funcs funcs[] = {
  { 'a',"Add partion",           add_part },
  { 'b',"Set bootable flag",     set_boot },
  { 'd',"Delete partition",      del_part },
  { 'l',"List partition types",  list_types },
  { 'p',"Print partition table", list_part },
  { 'q',"Quit fdisk",            quit },
  { 't',"Set partition type",    set_type },
  { 'w',"Write partition table", write_out },
  { '?',"Help",                  help },
  {  0,  NULL,                   NULL }
};

void quit()
{
  exit(1);
}

void list_part()
{
  if(*dev!=0)
    list_partition(NULL);
}

void list_types()  /* FIXME - Should make this more flexible */
{
  printf("\n0 Empty           5 Extended          81 Linux/Minix\n");
  printf("1 DOS 12-bit FAT  6 DOS 16bit >= 32M  82 Linux swap\n");
  printf("4 DOS 16bit <32M 80 Old Minix         83 Linux Native\n");
  fflush(stdout);
}

void add_part()
{
  char buf[8];
  unsigned char pentry[16];
  int part,scyl,ecyl;
  unsigned long tmp;
  unsigned char *oset;

  pentry[0]=0;

  printf("Add partition:\n\n");
  for(part=0;part<1 || part>4;) {
    printf("Which partition to add(1-4): ");
    fflush(stdout);
    fgets(buf,8,stdin);
    part=atoi(buf);
    if(*buf=='\n') return;
  }

  oset=partitiontable+(0x1be)+((part-1)*16);

  printf("geo.cyl=%d\n",geometry.cylinders);
  for(scyl=geometry.cylinders+1;scyl<0 || scyl>geometry.cylinders;) {
    printf("First cylinder(%d-%d):",0,geometry.cylinders);
    fflush(stdout);
    fgets(buf,8,stdin);
    scyl=atoi(buf);
    if(*buf=='\n') return;
  }

  pentry[1]=scyl==0?1:0;
  pentry[2]=1+((scyl >> 2) & 0xc0);
  pentry[3]=(scyl&0xff);

#ifdef PART_TYPE
  pentry[4]=PART_TYPE;
#else
  pentry[4]=0x81;
#endif

  for(ecyl=geometry.cylinders+1;ecyl<scyl || ecyl>geometry.cylinders;) {
    printf("Ending cylinder(%d-%d):",0,geometry.cylinders);
    fflush(stdout);
    fgets(buf,8,stdin);
    ecyl=atoi(buf);
    if(*buf=='\n') return;
  }

  pentry[5]=geometry.heads-1;
  pentry[6]=geometry.sectors+((ecyl >> 2) & 0xc0);
  pentry[7]=(ecyl&0xff);

  tmp=spc*(unsigned long)scyl;
  if(scyl==0) tmp=(unsigned long)geometry.sectors;

  pentry[11]=(unsigned char)((tmp>>24uL)&0x000000ffuL);
  pentry[10]=(unsigned char)((tmp>>16uL)&0x000000ffuL);
  pentry[ 9]=(unsigned char)((tmp>> 8uL)&0x000000ffuL);
  pentry[ 8]=(unsigned char)((tmp      )&0x000000fful);

  tmp=spc*(unsigned long)(ecyl-scyl+1);
  if(scyl==0) tmp-=(unsigned long)geometry.sectors;

  pentry[15]=(unsigned char)((tmp>>24uL)&0x000000ffuL);
  pentry[14]=(unsigned char)((tmp>>16uL)&0x000000ffuL);
  pentry[13]=(unsigned char)((tmp>> 8uL)&0x000000ffuL);
  pentry[12]=(unsigned char)((tmp      )&0x000000ffuL);

  printf("Adding partition");
  memcpy(oset,pentry,16);
  printf("\n");
  fflush(stdout);
}

void set_boot()
{
  int part,a;
  char buf[8];

  printf("Toggle bootable flag:\n\n");
  for(part=0;part<1 || part>4;) {
    printf("Which partition to toggle(1-4): ");
    fflush(stdout);
    fgets(buf,8,stdin);
    part=atoi(buf);
    if(*buf=='\n') return;
  }

  a=(0x1ae)+(part*16);
  partitiontable[a]=(partitiontable[a]==0x00?0x80:0x00);
}

void set_type()  /* FIXME - Should make this more flexible */
{
  int part,type,a;
  char buf[8];
  printf("Set partition type:\n\n");
  for(part=0;part<1 || part>4;) {
    printf("Which partition to toggle(1-4): ");
    fflush(stdout);
    fgets(buf,8,stdin);
    part=atoi(buf);
    if(*buf=='\n') return;
  }

  a=(0x1ae)+(part*16)+4;
  for(type=0;;) {
    printf("Set partition type to what? (l for list): ");
    fflush(stdout);
    fgets(buf,8,stdin);
    if(*buf=='l')
      list_types();
    else {
      type=atoi(buf);
      if(type==0 || type==1 || type==4 || type==5 || type==6 ||
         type==80 || type==81 || type==82 || type==83) break;
      if(*buf=='\n') return;
    }
  }
  if(type>=80) type+=48;

  partitiontable[a]=type;
}

void del_part()
{
  int part;
  char buf[8];

  printf("Delete partition:\n\n");
  for(part=0;part<1 || part>4;) {
    printf("Which partition to delete(1-4): ");
    fflush(stdout);
    fgets(buf,8,stdin);
    part=atoi(buf);
    if(*buf=='\n') return;
  }

  printf("Deleting partition %d..",part);
  memset(partitiontable+(0x1be)+((part-1)*16),0,16);
  printf("Done\n");
  fflush(stdout);
}

void write_out()
{
  int i;

  if(lseek(pFd,0L,SEEK_SET)!=0) {
    printf("ERROR!  Cannot seek to offset 0.\n");
  } else {
    if((i=write(pFd,partitiontable,512))!=512) {
      printf("ERROR!  Only wrote %d of 512 bytes to the partition table.\n",i);
      printf("        Table possibly corrupted.\n");
    } else {
      printf("Successfully wrote %d bytes to %s\n",i,dev);
    }
  }
  fflush(stdout);
}

void help()
{
  Funcs *tmp;

  printf("Key Description\n");
  for(tmp=funcs;tmp->cmd;tmp++)
    printf("%c   %s\n",tmp->cmd,tmp->help);
  fflush(stdout);
}

void list_partition(devname)
char *devname;
{
  unsigned char ptbl[512],*partition;
  int i;
  unsigned long seccnt;

  if(devname!=NULL) {
    if((pFd=open(devname,O_RDONLY))==-1) {
      printf("Error opening %s (%d)\n",devname,-pFd);
      exit(1);
    }
    if((i=read(pFd,ptbl,512))!=512) {
      printf("Unable to read first 512 bytes from %s, only read %d bytes\n",
             devname,i);
      exit(1);
    }
  } else {
    memcpy(ptbl,partitiontable,512);
  }

  printf("                      START                  END\n");
  printf("Device    Boot  Head Sector Cylinder   Head Sector Cylinder  Type  Sector count\n");
  for(i=0x1be;i<0x1fe;i+=16) {
    partition=ptbl+i;   /* FIXME /--- gotta be a better way to do this ---\ */
    seccnt=((unsigned long)partition[15] << 24uL) + \
	((unsigned long)partition[14] << 16uL) + \
	((unsigned long)partition[13] << 8uL) + \
	(unsigned long)partition[12];
    if(partition[5])
    printf("%s%d  %c     %2d   %2d   %4d         %2d   %2d     %4d      %2x    %10ld\n",
           devname==NULL?dev:devname,1+((i-0x1be)/16),
           partition[0]==0?' ':(partition[0]==0x80?'*':'?'),
           partition[1],                                /* Start head */
           partition[2]&0x3f,                           /* Start sector */
           partition[3] | ((partition[2] & 0xc0) << 2), /* Start cylinder */
           partition[5],                                /* End head */
           partition[6]&0x3f,                           /* End sector */
           partition[7] | ((partition[6] & 0xc0) << 2), /* End cylinder */
           partition[4],                                /* Partition type */
           seccnt);                                     /* Sector count */
  }
  if(devname!=NULL) close(pFd);
  fflush(stdout);
}

int main(argc,argv)
int argc;
char *argv[];
{
  int i, mode=MODE_EDIT;

  dev[0]=0;

  for(i=1;i<argc;i++) {
    if(*argv[i]=='/') {
      if(*dev!=0) {
        printf("Can only specify one device on the command line.\n");
        exit(1);
      } else {
        strncpy(dev,argv[i],256); /* FIXME - Should be some value from a header */
      }
    } else
    if(*argv[i]=='-') {
      switch(*(argv[i]+1)) {
        case 'l': mode=MODE_LIST;
                  break;
        default: printf("Unknown command: %s\n",argv[i]);
                 exit(1);
                 break;
      }
    } else
      die
  }

  if(argc==1)
#ifdef DEFAULT_DEV
    strncpy(dev,DEFAULT_DEV,256);
#else
    die;
#endif


  if(mode==MODE_LIST) {
    if(*dev!=0) {
      list_partition(dev);
    } else {
      list_partition("/dev/hda");
      list_partition("/dev/hdb");
      /* Other drives */
    }
    return(1);
  }

  if(mode==MODE_EDIT) {
    Funcs *tmp;
    int flag=0;
    char buf[CMDLEN];

    if((pFd=open(dev,O_RDWR))==-1) {
      printf("Error opening %s (%d)\n",dev,-pFd);
      exit(1);
    }

    if((i=read(pFd,partitiontable,512))!=512) {
      printf("Unable to read first 512 bytes from %s, only read %d bytes\n",
             dev,i);
      exit(1);
    }

    if (ioctl(pFd, HDIO_GETGEO, &geometry)) {
      printf("Error getting geometry of disk, exiting.\n");
      exit(1);
    }
    if(geometry.heads==0 && geometry.cylinders==0 && geometry.sectors==0) {
      printf("WARNING!  Read disk geometry as 0/0/0.  Things may break.\n");
    }

    printf("c/h/s=%d/%d/%d",geometry.cylinders,geometry.heads,geometry.sectors);
    fflush(stdout);

    for(;;) {
      printf("Command%s:",flag==0?" (? for help)":""); flag=1;
      fflush(stdout);
      *buf = 0;
      if (fgets(buf,CMDLEN-1,stdin)) {
        printf("\n");

        for(tmp=funcs;tmp->cmd;tmp++) {
          if (*buf==tmp->cmd) {
            tmp->func();
            break;
          }
        }
      }
    } /* for */
  }

  return(0);
}
