/*
 * This is a Xenix style hex dump command.
 * by Robert de Bath
 *
 * The 'reverse hex dump' option is an addition that allows a simple 
 * method of editing binary files.
 *
 * The overkill Linux 'hexdump' command can be configured to generate
 * the same format as this command by this shell macro:
 *
 * hd() { hexdump -e '"%06.6_ax:" 8/1 " %02x" " " 8/1 " %02x" "  " ' \
 *                -e '16/1 "%_p" "\n"' \
 *                "$@"
 *      }
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

FILE *fd;
FILE *ofd;
char *outfile = 0;
int reverse = 0;
int lastnum[16] = {-1};
long lastaddr = -1;
long offset = 0;

void usage(void)
{
   fprintf(stderr, "Usage: hd [-r] [-o outfile] [-sSkip_bytes] [[seg:off#size] [file]]\n");
   exit(1);
}

void
printline(long address, int *num, char *chr, int eofflag)
{
   int   j;

   if (lastaddr >= 0)
   {
      for (j = 0; j < 16; j++)
	 if (num[j] != lastnum[j])
	    break;
      if (j == 16 && !eofflag)
      {
	 if (lastaddr + 16 == address)
	 {
	    fprintf(ofd, "*\n");
	    fflush(ofd);
	 }
	 return;
      }
   }

   lastaddr = address;
   fprintf(ofd, "%06lx:", address);
   for (j = 0; j < 16; j++) {
      if (j == 8)
	 fputc(' ', ofd);
      if (num[j] >= 0)
	 fprintf(ofd, " %02x", num[j]);
      else
	 fprintf(ofd, "   ");
      lastnum[j] = num[j];
      num[j] = -1;
   }

   fprintf(ofd, "  %.16s\n", chr);
}


void do_fd(void)
{
   int   j, ch;
   char  buf[20];
   int   num[16];

   if (offset)
      fseek(fd, offset, 0);

   for (ch = 0; ch != EOF; offset += 16) {
      memset(buf, '\0', 16);
      for (j = 0; j < 16; j++)
	 num[j] = -1;
      for (j = 0; j < 16; j++) {
	 ch = fgetc(fd);
	 if (ch == EOF)
	    break;

	 num[j] = ch;
	 if (isprint(ch) && ch < 0x80)
	    buf[j] = ch;
	 else
	    buf[j] = '.';
      }
       if (j) printline(offset, num, buf, ch == EOF);
   }
}

void do_mem(char *spec)
{
   unsigned long seg = 0, off = 0;
   unsigned char __far *addr;
   long   count = 256;
   char  buf[20];
   int   num[16];

   sscanf(spec, "%lx:%lx#%ld", &seg, &off, &count);
   if (seg > 0xffff || off > 0xffff) {
        printf("Error: segment or offset larger than 0xffff\n");
        return;
   }

   addr = (unsigned char __far *)((seg << 16) | (unsigned short)off);
   offset = ((seg << 4) + (unsigned short)off);
   for ( ; count > 0; count -= 16, offset += 16) {
      int j, ch;
      memset(buf, '\0', 16);
      for (j = 0; j < 16; j++)
	 num[j] = -1;
      for (j = 0; j < 16; j++)
      {
	 ch = *addr++;

	 num[j] = ch;
	 if (isprint(ch) && ch < 0x80)
	    buf[j] = ch;
	 else
	    buf[j] = '.';
      }
      printline(offset, num, buf, 0);
   }
}


/*
 * This function takes output from hd and converts it back into a binary
 * file
 */

void do_rev_fd(void)
{
   char  str[160];
   char * ptr;
   int   c[16], i, nxtaddr, addr;
   int   zap_last = 1;

   for (i = 0; i < 16; i++)
      c[i] = 0;
   nxtaddr = 0;

   for (nxtaddr = 0;;)
   {
      if (fgets(str, sizeof(str), fd) == NULL)
	 break;

      str[57] = 0;
      ptr = str;

      if( *ptr == '*' ) zap_last = 0;
      if( *ptr != ':' ) {
	 if( !isxdigit(*ptr) ) continue;
	 addr = strtol(ptr, &ptr, 16);
      }
      else 
	 addr = nxtaddr;
      if( *ptr == ':' ) ptr++;

      if (nxtaddr == 0)
	 nxtaddr = addr;
      if( zap_last ) memset(c, 0, sizeof(c));
      else zap_last = 1;
      while (nxtaddr < addr)
      {
	 for (i = 0; nxtaddr < addr && i < 16; i++, nxtaddr++)
	    fputc(c[i], ofd);
      }
      for (i = 0; i < 16 && *ptr; i++)
      {
	 char * ptr2;
	 c[i] = strtol(ptr, &ptr2, 16);
	 if( ptr == ptr2 ) break;
	 fputc(c[i], ofd);
	 ptr = ptr2;
      }
      nxtaddr += 16;
   }
}

int main(int argc, char **argv)
{
   int   done = 0;
   int   ar;
   int   aflag = 1;

   ofd = stdout;

   for (ar = 1; ar < argc; ar++)
      if (aflag && argv[ar][0] == '-')
	 switch (argv[ar][1]) {
	 case 'r': /* Reverse */
	    reverse = 1;
	    break;
	 case 's': /* Skip */
	    offset = strtol(argv[ar] + 2, (void *) 0, 0);
	    break;
	 case '-':
	    aflag = 0;
	    break;
	 case 'o': /* Output */
	    if( argv[ar][2] ) outfile = argv[ar]+2;
	    else {
	       if( ++ar >= argc ) usage();
	       outfile = argv[ar];
	    }
	    break;
	 default:
	    usage();
	 }
      else {
         if( outfile ) {
	    if( ofd != stdout ) fclose(ofd);
	    ofd = fopen(outfile, "w");
	    if( ofd ==  0 ) {
	       fprintf(stderr, "Cannot open file '%s'\n", outfile);
	       exit(9);
	    }
	 }
	 if (strchr(argv[ar], ':'))
	   do_mem(argv[ar]);
	 else {
	    fd = fopen(argv[ar], "rb");
	    if (fd == 0)
	       fprintf(stderr, "Cannot open file '%s'\n", argv[ar]);
	    else {
	       if (reverse)
	          do_rev_fd();
	       else
	          do_fd();
	       fclose(fd);
	    }
         }
	 done = 1;
      }

   if (!done) {
      fd = stdin;
      if( reverse )
         do_rev_fd();
      else
         do_fd();
   }
   return 0;
}
