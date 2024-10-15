/*  Copyright 2003 Stefan Feuz, Lukas Meyer, Thomas Locher
 *  Copyright 2004,2006,2007,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Filename:
 *    mclasserase.c
 *
 * Original Creation Date:
 *    05.III.2003
 *
 * Copyright:
 *    GPL
 *
 * Programmer:
 *    Stefan Feuz, Lukas Meyer, Thomas Locher
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "vfat.h"
#include "mainloop.h"
#include "fsP.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "file.h"

#include <unistd.h>
#include <stdio.h>

/**
 * Prints the Usage Message to STDOUT<br>
 *
 * @author  stefan feuz<br>
 *          stefan.feuz@ruag.com
 *
 * @param   n.a.
 *
 * @returns n.a.
 *
 */
static void usage(int ret) NORETURN;
static void usage(int ret)
{
  fprintf(stderr, "Mtools version %s, dated %s\n", mversion, mdate);
  fprintf(stderr, "Usage: %s [-d] drive:\n", progname);
  exit(ret);
}

/**
 * Delete all files on a Drive.<br>
 *
 * @author  Lukas Meyer<br>
 *          lukas.meyer@ruag.com
 * @version 0.4, 11.12.2003
 *
 * @param   drive   the drive to erase
 * @param   debug   1: stop after each erase cycle, 0: normal mode
 * 
 * @returns n.a.
 *
 */
static void do_mclasserase(char drive,int debug) NORETURN;
static void do_mclasserase(char drive,int debug)
{
  struct device dev;		/* Device information structure */
  union bootsector boot;

  int media;			/* Just used to enter some in find_device */
  char name[EXPAND_BUF];
  Stream_t *Stream;
  struct label_blk_t *labelBlock;
  
  FILE * fDevice;              /* Stores device's file descriptor */
  
  char cCardType[12];
  
  char drivel[3];		/* Stores the drive letter */


  int i = 0;

  /* FILE *forf; */

  char dummy[2];       /* dummy input for debugging purposes.. */
  int icount=0;
  int iTotalErase = 0;

/* How many times we'll overwrite the media: */
#define CYCLES 3
  unsigned char odat[CYCLES];	/* Data for each overwrite procedure */

  /* Creating values for overwrite  */
  odat[0]=0xff;
  odat[1]=0x00;
  odat[2]=0xff;
  

  if (debug == 1)
     printf("cycles: %i, odats: %i,%i,%i\n",CYCLES,odat[0],odat[1],odat[2]);
  
  

  /* Reading parameters from card. Exit with -1 if failed. */
  if(! (Stream = find_device(drive, O_RDONLY, &dev, &boot,
					   name, &media, 0, NULL)))
	exit(1);

  FREE(&Stream);

  /* Determine the FAT - type */
#if 0
  if(WORD(fatlen)) {
    labelBlock = &bbelBlock = &boot->ext.old.labelBlock;
  } else {
    labelBlock = &boot->ext.fat32.labelBlock;
  }
#endif

  /* we use only FAT12/16 ...*/
  labelBlock = &boot.boot.ext.old.labelBlock;
   
  /* store card type */
  sprintf(cCardType, "%11.11s", labelBlock->label);

  if (debug == 1)
  {
     printf("Using Device: %s\n",name);
     printf("Card-Type detected: %s\n",cCardType);
  }
      
  /* Forming cat command to overwrite the medias content. */
  sprintf( drivel, "%c:", ch_tolower(drive) );

#if 0
  media_sectors = dev.tracks * dev.sectors;
  sector_size = WORD(secsiz) * dev.heads; 
  

	printf(mcat);
	printf("\n%d\n", media_sectors);
	printf("%d\n", sector_size);
#endif
 
  /*
   * Overwrite device
   */
  for( i=0; i < CYCLES; i++){

     if (debug==1)
     {
        printf("Erase Cycle %i, writing data: 0x%2.2x...\n",i+1,odat[i]);
     }

     fDevice = fopen(name,"ab+");
          
     if (fDevice == 0)
     {
        perror("Error opening device");
	exit(-1);
     }


     if (debug==1)
     {
        printf("Open successful...\n");
	printf("Flushing device after 32 kBytes of data...\n");
	printf("Erasing:");
	fflush( stdout );
     }

     /* iTotalErase = 0; */
      
     /*
      * overwrite the whole device
      */
     while ((feof(fDevice)==0) && (ferror(fDevice)==0))
     {
          		
	fputc(odat[i],fDevice);

	icount++;
	if (icount > (32 * 1024))
	{
	   /* flush device every 32KB of data...*/
	   fflush( fDevice );
           
	   iTotalErase += icount;
	   if (debug == 1)
	   {
	      printf(".");	   
	      fflush( stdout );
	   }
	   icount=0;
	}
     }
     
     if (debug==1)
     {
        printf("\nPress <ENTER> to continue\n");
        printf("Press <x> and <ENTER> to abort\n");
	
	if(scanf("%c",dummy) < 1)
	  printf("Input error\n");
	fflush( stdin );
	
	if (strcmp(dummy,"x") == 0)
	{
	   printf("exiting.\n");
	   exit(0);
	}
     }
     
     fclose(fDevice);

  }

   
  /*
   * Format device using shell script
   */  
  if (debug == 0)
  {
  	/* redirect STDERR and STDOUT to the black hole... */
	if (dup2(open("/dev/null", O_WRONLY), STDERR_FILENO) != STDERR_FILENO)
		printf("Error with dup2() stdout\n");
	if (dup2(open("/dev/null", O_WRONLY), STDOUT_FILENO) != STDOUT_FILENO)
	        printf("Error with dup2() stdout\n");
  }
	       
  if (debug == 1)
      printf("Calling amuFormat.sh with args: %s,%s\n",cCardType,drivel);
	
  execlp("amuFormat.sh","",cCardType,drivel,NULL);

  /* we never come back...(we shouldn't come back ...) */
  exit(-1);
    
}


/**
 * Total Erase of Data on a Disk. After using mclasserase there won't
 * be ANY bits of old files on the disk.<br>
 * </b>
 * @author  stefan feuz<br>
 *          thomas locher<br>
 *          stefan.feuz@ruag.com
 *          thomas.locher@ruag.com
 * @version 0.3, 02.12.2003
 *
 * @param   argc    generated automatically by operating systems
 * @param   **argv1  generated automatically by operating systems
 * @param   type    generated automatically by operating systems
 *
 * @param   -d      stop after each erase cycle, for testing purposes
 * 
 * @returns int     0 if all is well done<br>
 *          int     -1 if there is something wrong
 *
 * @info    mclasserase [-p tempFilePath] [-d] drive:
 *
 *
 */
void mclasserase(int argc, char **argv, int type UNUSEDP) NORETURN;
void mclasserase(int argc, char **argv, int type UNUSEDP)
{
  /* declaration of all variables */
  int c;
  int debug=0;
  /* char* tempFilePath=NULL; */
  char drive='a';

  extern int optind;

  destroy_privs();

  /* check and read command line arguments */
#ifdef DEBUG
  printf("mclasserase: argc = %i\n",argc);
#endif
  /* check num of arguments */
  if(helpFlag(argc, argv))
    usage(0);
  if ( (argc != 2) & (argc != 3) & (argc != 4))
    { /* wrong num of arguments */
    printf ("mclasserase: wrong num of args\n");
    usage(1);
  }
  else
  { /* correct num of arguments */
    while ((c = getopt(argc, argv, "+p:dh")) != EOF)
    {
      switch (c)
      {
	
	case 'd':
           
	   printf("=============\n");
	   printf("Debug Mode...\n");
	   printf("=============\n");
           debug = 1;
	   break;
	case 'p':
           printf("option -p not implemented yet\n"); 
           break;
	case 'h':
	  usage(0);
        case '?':
           usage(1);
        default:
           break;
       }
     }
#ifdef DEBUG
     printf("mclasserase: optind = %i\n",optind);
   /* look for the drive to erase */
   printf("mclasserase: searching drive\n");
#endif
   for(; optind < argc; optind++)
   {
     if(!argv[optind][0] || argv[optind][1] != ':')
     {
       usage(1);
     }
     drive = ch_toupper(argv[optind][0]);
   }
  }
#ifdef DEBUG
  printf("mclasserase: found drive %c\n", drive);
#endif 
  /* remove all data on drive, you never come back if drive does
   * not exist */
  
  do_mclasserase(drive,debug);
}
