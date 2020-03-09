/*******************************************************************************
***
*** Filename         : imgdump.c
*** Purpose          : Dumps the contents of Psion .img files as hex/ASCII,
***                    and interprets the image header.
*** Author           : Matt J. Gumbley
*** Created          : 21/01/97
*** Last updated     : 27/01/97
***
********************************************************************************
***
*** Modification Record
*** 11/03/97 SNH MS-DOS Port
***
*******************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef __TSC__
#include <unistd.h>
#else
#include <io.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#include "img.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

/*******************************************************************************
***
*** Function         : hexdig
*** Preconditions    : 0 <= num <= 15
*** Postconditions   : num is converted into its corresponding hexadecimal digit
***
*******************************************************************************/

char hexdig(int num)
{
    static char digs[] = "0123456789ABCDEF";
    return num < 16 ? digs[num] : '?';
}

/*******************************************************************************
***
*** Function         : hexdumpbuf
*** Preconditions    : buf points to an area of memory and len is the length of
***                    this area
*** Postconditions   : An offset | hexadecimal | printable ASCII dump of the
***                    designated memory area is output
***
*******************************************************************************/

void hexdumpbuf(unsigned char *buf, long buflen)
{
    char LINE[80];
    long offset = 0L;
    long left = buflen;
    int i, upto16, x;
    unsigned char b;
    while (left > 0) {
	for (i = 0; i < 78; i++)
	    LINE[i] = ' ';
	LINE[9] = LINE[59] = '|';
	LINE[77] = '\0';
	LINE[78] = '\0';
	sprintf(LINE, "%08lX", offset);
	LINE[8] = ' ';
	upto16 = (left > 16) ? 16 : (int) left;
	for (x = 0; x < upto16; x++) {
	    b = buf[offset + x];
	    LINE[11 + (3 * x)] = hexdig((b & 0xf0) >> 4);
	    LINE[12 + (3 * x)] = hexdig(b & 0x0f);
	    LINE[61 + x] = isprint((char) b) ? ((char) b) : '.';
	}
	puts(LINE);
	offset += 16L;
	left -= 16L;
    }
}


void hexdump(unsigned char *bufstart, unsigned char *buf, long buflen,
	     unsigned char *bufend)
{
    if (buf + buflen > bufend) {
	fprintf(stderr,
		"*** Requested dump would exceed file length: dump truncated ***\n");
	buflen = bufend - buf;
    }
    printf("dump is from offset 0x%08X in the file, for 0x%08lX bytes\n",
	   buf - bufstart, buflen);
    hexdumpbuf(buf, buflen);
}


void imagefiletype(unsigned char *buf, long buflen)
{
    int i;
    unsigned char *bufend = buf + buflen;
    ImgHeader *ifh = (ImgHeader *) buf;

    printf("ImageFileType\nHeader dump:\n");
    hexdump(buf, buf, sizeof(ImgHeader), bufend);
    printf("Header contents:\n");
    printf("ImageVersion    0x%04X\n", ifh->ImageVersion);
    printf("HeaderSizeBytes 0x%04X\n", ifh->HeaderSizeBytes);
    printf("CodeParas       0x%04X\n", ifh->CodeParas);
    printf("InitialIP       0x%04X\n", ifh->InitialIP);
    printf("StackParas      0x%04X\n", ifh->StackParas);
    printf("DataParas       0x%04X\n", ifh->DataParas);
    printf("HeapParas       0x%04X\n", ifh->HeapParas);
    printf("InitialisedData 0x%04X bytes\n", ifh->InitialisedData);

    /* printf("=>Uninitialised 0x%04X bytes\n", ifh->DataParas*16 - ifh->InitialisedData);
     * not entirely sure about this -- MJG */

    printf("CodeCheckSum    0x%04X\n", ifh->CodeCheckSum);
    printf("DataCheckSum    0x%04X\n", ifh->DataCheckSum);
    printf("CodeVersion     0x%04X\n", ifh->CodeVersion);
    printf("Priority        0x%04X\n", ifh->Priority);
    for (i = 0; i < MaxAddFiles; i++) {
	printf("Add[%d].offset   0x%04X\n", i, ifh->Add[i].offset);
	printf("Add[%d].length   0x%04X\n", i, ifh->Add[i].length);
    }
    printf("DylCount        0x%04X\n", ifh->DylCount);
    printf("DylTableOffset  0x%08lX\n",
	   (long) (ifh->DylTableOffsetLo + (65536 * ifh->DylTableOffsetHi)));
    printf("Spare           0x%04X\n", ifh->Spare);

    printf("Code dump:\n");
    hexdump(buf, buf + ifh->HeaderSizeBytes, ifh->CodeParas * 16, bufend);

    printf("Data dump:\n");
    hexdump(buf, buf + ifh->HeaderSizeBytes + (ifh->CodeParas * 16),
	    ifh->InitialisedData, bufend);

    for (i = 0; i < MaxAddFiles; i++) {
	if (ifh->Add[i].length != 0) {
	    printf("Embedded file %d dump:\n", i);
	    hexdump(buf, buf + ifh->Add[i].offset, ifh->Add[i].length, bufend);
	}
    }
}

void lddfiletype(unsigned char *buf, long buflen)
{
    int i;
    ImgHeader *ifh = (ImgHeader *) (buf + 16);
    unsigned char *bufend = buf + buflen;
    printf("LDDFileType\n");
    printf("ImageVersion    0x%04X\n", ifh->ImageVersion);
    printf("HeaderSizeBytes 0x%04X\n", ifh->HeaderSizeBytes);
    printf("CodeParas       0x%04X\n", ifh->CodeParas);
    printf("InitialIP       0x%04X\n", ifh->InitialIP);
    printf("StackParas      0x%04X\n", ifh->StackParas);
    printf("DataParas       0x%04X\n", ifh->DataParas);
    printf("HeapParas       0x%04X\n", ifh->HeapParas);
    printf("InitialisedData 0x%04X\n", ifh->InitialisedData);
    printf("CodeCheckSum    0x%04X\n", ifh->CodeCheckSum);
    printf("DataCheckSum    0x%04X\n", ifh->DataCheckSum);
    printf("CodeVersion     0x%04X\n", ifh->CodeVersion);
    printf("Priority        0x%04X\n", ifh->Priority);
    for (i = 0; i < MaxAddFiles; i++) {
	printf("Add[%d].offset  0x%04X\n", i, ifh->Add[i].offset);
	printf("Add[%d].length  0x%04X\n", i, ifh->Add[i].length);
    }
    printf("DylCount        0x%04X\n", ifh->DylCount);
    printf("DylTableOffset  0x%08lX\n",
	   (long) (ifh->DylTableOffsetLo + (65536 * ifh->DylTableOffsetHi)));
    printf("Spare           0x%04X\n", ifh->Spare);
    hexdump(buf, buf + sizeof(ImgHeader), buflen - sizeof(ImgHeader), bufend);
}

/*void dylfiletype(unsigned char *buf, long buflen)
{
  struct dylfheader {
    word16 words[8];
  } *dylfh;
  int i;
  printf("DYLFileType\n");
  dylfh=(struct dylfheader *)(buf+2);
  for (i=0; i<8; i++)
    printf("word %02d = 0x%04X\n",i+1,dylfh->words[i]);
  hexdump(buf+0x02, buflen-0x02);
}*/


/* This allows me to do the file in chunks quite nicely */
void analyse(unsigned char *buf, long buflen)
{
    /* What sort of file is this? */
    if (buflen > 16 && strncmp("ImageFileType**", buf, 15) == 0)
	imagefiletype(buf, buflen);
    /*else if (buflen>2 && buf[0] == 0x41 && buf[1] == 0xdd)
     * dylfiletype(buf, buflen);
     * if (buflen>16 && strncmp("LDDFileType****",buf,15) == 0)
     * lddfiletype(buf, buflen); */

    else
	hexdump(buf, buf, buflen, buf + buflen);
}


void imagedump(char *filename)
{
    int fd;
    unsigned long filesize, nread;
    unsigned char *buf;

    fd = open(filename, O_RDONLY | O_BINARY);
    if (fd == -1) {
	fprintf(stderr, "imgdump: Can't open %s\n", filename);
	return;
    }

    filesize = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    printf("%s is %ld (0x%lx) bytes in size\n", filename, filesize, filesize);

    buf = malloc(filesize);
    if (buf == NULL) {
	fprintf(stderr,
		"imgdump: Can't allocate buffer of %ld bytes for %s.\n",
		filesize, filename);
	close(fd);
	return;
    }

    nread = read(fd, buf, filesize);
    close(fd);

    if (nread != filesize) {
	fprintf(stderr,
		"imgdump: Only read %ld bytes of %s - should have read %ld.\n",
		nread, filename, filesize);
	return;
    }

    analyse(buf, filesize);
    free(buf);
}


int main(int argc, char *argv[])
{
    int i;

    if (argc == 1)
	fprintf(stderr, "usage: imgdump .img-file1 .... .img-fileN\n");
    else
	for (i = 1; i < argc; i++)
	    imagedump(argv[i]);
    exit(0);
}
