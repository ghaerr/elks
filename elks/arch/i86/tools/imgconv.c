/*******************************************************************************
***
*** Filename         : imgconv.c
*** Purpose          : converts a.out executables produced by bcc/as86/ld86
***                    to the Psion .img format. This program is invoked when 
***                    linking from bcc if the -img option is used.
*** Author(s)        : Matt J. Gumbley <matt@cs.keele.ac.uk>
***                    This program was pieced together from postings to the
***                    comp.sys.psion.programmer newsgroup, in particular, I'd 
***                    like to thank the following for their efforts in 
***                    discovering and publishing the format of the .img file:
***                    Dr. Olaf Flebbe 
***                      <o.flebbe@science-computing.uni-tubingen.de>
***                      (for writing an initial version of this; his comments
***                       left intact)
***                    Alasdair Manson <ali-manson@psion.com>
***                      (for publishing the .img file structure)
***                    Andrew Lord     <Andrew_Lord@tertio.co.uk>
***                      (for asking about the .img file and investigating 
***                       bcc's suitability for developing Psion programs)
***                    Nic Wise        <email address to be supplied later!>
***                      (for porting the toolset to DOS, pointing out 
***                       incompatibilities.)
***                    Keith Walker <kdw@oasis.icl.co.uk>
***                      (CPOC author, with Dave Walker, for help with .img
***                       file format, magic statics, initialisation)
***                    Dr. Stephen Henson  <shenson@bigfoot.com>
***                      (Corrections to the header structure, etc.)
***
***                    This program is placed under the GNU public license.
***
*** Created          : 19/01/97, probably
*** Last updated     : 11/03/97
***
********************************************************************************
***
*** Modification Record
*** 27/01/97 MJG Changed mmap() to standard IO calls, since DOS doesn't have
***              mmap().  Added command line options, and tidied up.
*** 05/03/97 SH  Header corrections, casting.
*** 09/03/97 MJG Now builds in the embedded file information, given a list in
***              a .afl file with the same base name as the .img file.
*** 11/03/97 SNH MS-DOS port
***
*******************************************************************************/


/* Olaf's comments: ************************************************************
Hi,

I wrote this little converter for the minix/linux ld86 linker. It
converts the BSD a.out to Psion IMG. I was able to crash the Psion
emulator with a simple image containing the `ret' instruction ;-)

The IMG Format was reverse engeniered by me. So, please comment if anybody
knows details....for instance what do the magic numbers mean?

BTW: Is it true, that the Psion Img format knows no .bss segment? 
(.bss==uninitialzed data segment)

The code compiles on Linux. The Header "bsd-a.out.h" is from
the bin86-0.1 source code distribution by Bruce Evans changed by H.J.Lu.

*/

/* The format of the .img file is in img.h, as supplied by Alasdair,
the magic has been demystified, yes, we have uninitialised data (.bss)
and we're using the x86_aout.h definition now.

***********************************************************  MJG  *************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef __TSC__
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "img.h"
#include "a.out.h" /* A symbolic link to ../ld/x86_aout.h */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif


struct embedded {
  unsigned short length;
  unsigned char *data;
} embeddedFiles[MaxAddFiles];



short chksum( unsigned char *a, int len) {
  short sum = 0;

  for ( ; len > 0 ; len--)
    sum += *a++;
  return sum;
}


int main( int argc, char *argv[]) {
  
  int a_out_fd;
  struct stat statbuf;
  char *a_out;
  struct exec *header;
  /*short img_ar[12];*/
  ImgHeader imgHeader;
  FILE *img_fd;
  /* Embedded file info */
  char aflName[20];            /* Name of file containing list of added files */
  char afName[20];                    /* Name of embedded file from .afl file */
  int af;                   /* general purpose integer for add files routines */
  FILE *afl_fd;                                              /* The .afl FILE */
  int af_fd;                                            /* Each embedded file */
  struct stat afstatbuf;                                /* Each embedded file */
  unsigned short embeddedTotalLength, embeddedOffset;
  int afLen,numEmbeddedFiles;
  
  
  /* Default values for the configurable options */
  unsigned short HeapParas = 128;
  unsigned short StackParas = 64; /* Is this the default SIBO SDK uses? */
  unsigned short InitialIP = 0;
  unsigned short Priority = 128;
  unsigned short CodeVersion = 8204;
  int verbose = FALSE;

  int i;

  if (argc < 3) 
    goto usage;
  
  if ((a_out_fd = open( argv[1], O_RDONLY|O_BINARY)) < 0) {
    fprintf( stderr, "imgconv: Couldn't open input file %s\n", argv[1]);
    exit(1);
  }

  if (fstat( a_out_fd, &statbuf) < 0) {
    fprintf( stderr, "imgconv: Couldn't fstat input file %s: ", argv[1]);
    perror("");
    exit(1);
  }

  if (( a_out = malloc(statbuf.st_size)) == NULL) {
    fprintf(stderr, "imgconv: Couldn't allocate %ld bytes to read input file %s\n", statbuf.st_size, argv[1]);
    exit(1);
  }

  if ( statbuf.st_size != read( a_out_fd, a_out, statbuf.st_size)) {
    fprintf( stderr, "imgconv: Couldn't read %ld bytes of input file %s: ", statbuf.st_size, argv[1]);
    perror("");
    exit(1);
  }
#ifdef MSDOS
  if (NULL == (img_fd = fopen( argv[2] , "wb"))) {
#else
  if (NULL == (img_fd = fopen( argv[2] , "w"))) {
#endif
    fprintf( stderr, "imgconv: Couldn't open %s for writing\n", argv[1]);
    exit(1);
  }
 
  /* Handle any extra command line options */
  for (i=3; i<argc; i++) {
    if (argv[i][0] != '-')  {
      fprintf(stderr, "imgconv: %s is not a - option\n",argv[i]);
      goto usage;
    }
    switch (tolower(argv[i][1])) {
      case 'c': /* Specify code version */
        if (sscanf(&argv[i][2], "%hu", &CodeVersion) != 1) 
          goto usage;
        break;
      case 'h': /* Specify heap paragraphs */
        if (sscanf(&argv[i][2], "%hu", &HeapParas) != 1) 
          goto usage;
        break;
      case 'i': /* Specify initial IP */
        if (sscanf(&argv[i][2], "%hu", &InitialIP) != 1) 
          goto usage;
        break;
      case 'p': /* Specify priority */
        if (sscanf(&argv[i][2], "%hu", &Priority) != 1) 
          goto usage;
        break;
      case 's': /* Specify stack paragraphs */
        if (sscanf(&argv[i][2], "%hu", &StackParas) != 1) 
          goto usage;
        break;
      case 'v': /* Verbose */
        verbose = TRUE;
        break;
      default:
        fprintf(stderr, "imgconv: unknown option %s\n", argv[i]);
        goto usage;
    }
  }

  header = (struct exec *) a_out;

  if (BADMAG( *header)) {
    fprintf( stderr, "imgconv: Bad Magic in input file %s\n", argv[1]);
    exit(1);
  }
  if (verbose)
    printf( "%s info:\nText 0x%04lx, Data 0x%04lx, BSS 0x%04lx\n", 
            argv[1], header->a_text, header->a_data, header->a_bss );
  
  a_out += header->a_hdrlen;


  /* Do we have an add file list? (.afl file) Try the base name of the
     .img file, replacing .img with .afl... */
  strcpy(aflName, argv[2]);
  for (af=0; af<16 && aflName[af]!='.'; af++);
  strcpy(aflName+af, ".afl");

  embeddedTotalLength = numEmbeddedFiles = 0;

#ifdef MSDOS
  if (NULL != (afl_fd = fopen( aflName, "rb"))) {
#else
  if (NULL != (afl_fd = fopen( aflName, "r"))) {
#endif
    if (verbose)
      printf("Adding embedded files from list held in %s\n",aflName);

    
    for (af=0; af<MaxAddFiles; af++) {

      embeddedFiles[af].length=0;
      embeddedFiles[af].data=NULL;
   
      /* What is the name of this embedded file? */
      if (fgets(afName, 20, afl_fd) == NULL)
        continue;

      afLen = strlen(afName);

      if (afLen < 2)
        continue;

      if (afName[afLen-1] == '\n')
        afName[afLen-1]='\0';

      if ((af_fd = open( afName, O_RDONLY|O_BINARY)) < 0) {
        fprintf( stderr, "imgconv: Couldn't open embedded file %s\n", afName);
        exit(1);
      }

      if (fstat( af_fd, &afstatbuf) < 0) {
        fprintf( stderr, "imgconv: Couldn't fstat embedded file %s: ", afName);
        perror("");
        exit(1);
      }

      /* Embedded files can be zero length */
      embeddedFiles[af].length = afstatbuf.st_size;
      if (embeddedFiles[af].length > 0) {

        /* Allocate storage space */
        if ((embeddedFiles[af].data=malloc(embeddedFiles[af].length))==NULL) {
          fprintf(stderr, 
            "imgconv: Couldn't allocate %d bytes to read embedded file %s\n", 
            embeddedFiles[af].length, afName);
          exit(1);
        }

        if (embeddedFiles[af].length !=
            read(af_fd, embeddedFiles[af].data, embeddedFiles[af].length)) {
          fprintf(stderr, 
            "imgconv: Couldn't read %d bytes of embedded file %s: ", 
            embeddedFiles[af].length, afName);
          perror("");
          exit(1);
        }
      }
      close(af_fd);
      numEmbeddedFiles++;
      embeddedTotalLength += embeddedFiles[af].length;
      if (verbose)
        printf("Read in %d bytes from %s\n",embeddedFiles[af].length,afName);
    }
    if (verbose)
      printf("Embedded files take up %d bytes\n", embeddedTotalLength);
  }
 

  /* Olaf's original .img header...
  fwrite( "ImageFileType**", 16, 1, img_fd);
  img_ar[ 0] = 8207;
  img_ar[ 1] = 64;
  img_ar[ 2] = header->a_text / 16;
  img_ar[ 3] = 0;
  img_ar[ 4] = 1000;  ** Stack Size **
  img_ar[ 5] = header->a_data / 16;
  img_ar[ 6] = 128;
  img_ar[ 7] = header->a_data;
  img_ar[ 8] = chksum( (short *) a_out, header->a_text);
  img_ar[ 9]  = chksum( (short *)(a_out+header->a_text), header->a_data);
  img_ar[ 10] = 128;  ** Start up Priority **
  img_ar[ 11] = 8204; ** Version ?? **
  */

  strcpy(imgHeader.Signature, "ImageFileType**");
  imgHeader.ImageVersion    = 0x200f;
  imgHeader.HeaderSizeBytes = sizeof(ImgHeader) + embeddedTotalLength;
  imgHeader.CodeParas       = header->a_text / 16;
  imgHeader.InitialIP       = InitialIP;
  imgHeader.StackParas      = StackParas;
  imgHeader.DataParas       = (header->a_data + header->a_bss + 15) / 16;
  imgHeader.HeapParas       = HeapParas;
  imgHeader.InitialisedData = header->a_data; 
  imgHeader.CodeCheckSum    = chksum((unsigned char *) a_out, header->a_text);
  imgHeader.DataCheckSum    = chksum((unsigned char *) (a_out + header->a_text), header->a_data);
  imgHeader.CodeVersion     = CodeVersion;
  imgHeader.Priority        = Priority;

  embeddedOffset = embeddedTotalLength > 0 ? sizeof(ImgHeader) : 0;

  for (i=0; i<MaxAddFiles; i++) {
    imgHeader.Add[i].offset = i<numEmbeddedFiles ? embeddedOffset : 0;
    imgHeader.Add[i].length = i<numEmbeddedFiles ? embeddedFiles[i].length : 0;
    embeddedOffset += embeddedFiles[i].length;
  }

  imgHeader.DylCount        = 0;
  imgHeader.DylTableOffsetLo= 0;
  imgHeader.DylTableOffsetHi= 0;
  imgHeader.Spare           = 0;

  /* Olaf missed the DylTableOffset off this, methinks...
   * fwrite( img_ar, 12, 2, img_fd);
   * for (i=0 ; i < 12; i++) 
   *   img_ar[i] = 0;
   * fwrite( img_ar, 12, 2, img_fd);*/

  /* Write out the header structure */
  fwrite((char *)&imgHeader, sizeof(imgHeader), 1, img_fd);

  /* Write out the embedded files, if any */
  for (i=0; i<MaxAddFiles; i++) 
    if (embeddedFiles[i].length > 0) {
      fwrite((char *)embeddedFiles[i].data, embeddedFiles[i].length, 1, img_fd);
      free(embeddedFiles[i].data);
    }
      
  /* Write out the code segment */
  fwrite( a_out, header->a_text, 1, img_fd);

  /* Write out the initialised data segment */
  fwrite( a_out+header->a_text, header->a_data, 1, img_fd);

  fclose( img_fd);
  close(a_out_fd);
  exit(0);

usage: /* Vile, nasty hack: I love it :) */
  fprintf(stderr, "usage: imgconv a.out-file .img-file [options]\n\n");
  fprintf(stderr, "imgconv converts an executable file in a.out format (as produced\n");
  fprintf(stderr, "by the bcc toolset) into a Psion-compatible .img file.\n\n");
  fprintf(stderr, "These options can be passed to this program with the -Y\n");
  fprintf(stderr, "option of bcc, e.g. bcc prog.c -o prog.img -Y-p50\n\n");
  fprintf(stderr, "options: all <num>'s are in decimal, defaults in brackets\n");
  fprintf(stderr, " -C<num>   Specify code version number        (8204)\n");
  fprintf(stderr, " -H<num>   Specify number of heap paragraphs  (128)\n");
  fprintf(stderr, " -I<num>   Specify initial value for IP       (0)\n");
  fprintf(stderr, " -P<num>   Specify priority                   (128)\n");
  fprintf(stderr, " -S<num>   Specify number of stack paragraphs (64)\n");
  fprintf(stderr, " -V        Verbose mode: display code stats   (Don't)\n\n");
  fprintf(stderr, "Up to four files may be embedded inside the resultant\n");
  fprintf(stderr, ".img file by placing their filenames on separate lines\n");
  fprintf(stderr, "in a .afl file. This .afl file must have the same base\n");
  fprintf(stderr, "name as the .img file. e.g. Create a file fred.afl containing:\n");
  fprintf(stderr, "  fred.pic\n  fred.rsc\n  fred.shd\n");
  fprintf(stderr, "To have these files added to the fred.img file.\n");
  exit(1);
}


/*
  Dr. Olaf Flebbe                            Phone +49 (0)7071-9457-32
  science + computing gmbh                     FAX +49 (0)7071-9457-27
  Hagellocher Weg 71
  D-72070 Tuebingen  Email: o.flebbe@science-computing.uni-tuebingen.de
*/ 

