/*
 * ELKS a.out binary compressor
 *
 * Apr 2021 Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include "minix.h"

struct minix_exec_hdr mh;
struct elks_supl_hdr eh;
typedef unsigned short elks_size_t;
char *infile, *outfile;
char exomizer_binary[] = "exomizer/src/exomizer";

static char *readsection(int fd, int size)
{
	int n;
	char *p;

	if (size == 0)
		return NULL;

	if ((p = malloc(size)) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}
	if ((n = read(fd, p, size)) != size)
	{
		printf("Error reading section: %s\n", infile);
		exit(1);
	}
	return p;
}

static void writesection(int fd, void *section, int size)
{
	if (write(fd, section, size) != size)
	{
		printf("Write error: %s\n", outfile);
		close(fd);
		unlink(outfile);
		exit(1);
	}
}

static void copyrest(int ifd, int ofd)
{
	int size;
	char buf[1024];

	while ((size = read(ifd, buf, 1024)) > 0)
	{
		if (write(ofd, buf, size) != size)
		{
error:
			printf("Write error: %s\n", outfile);
			close(ofd);
			unlink(outfile);
			exit(1);
		}
	}
	if (size != 0) goto error;
}

int main(int ac, char **av)
{
	int ifd, ofd, efd, n;
	long orig_size, compr_size;
	elks_size_t sztext, szdata, szftext;
	elks_size_t compr_sztext, compr_szdata, compr_szftext;
	char *text, *data, *ftext;
	char *compr_text, *compr_data, *compr_ftext;
	struct stat sbuf;
	char *exomizer_outfile = "ex.out";
	int do_text = 1;
	int do_ftext = 1;
	int do_data = 1;
	char cmd[256];

	if (ac != 3)
	{
		printf("Usage: elks-compress infile outfile\n");
		exit(1);
	}
	infile = av[1];
	outfile = av[2];

	if ((ifd = open(infile, O_RDONLY)) < 0)
	{
		printf("Can't open %s\n", infile);
		exit(1);
	}

	n = read(ifd, &mh, sizeof(mh));
	if (n != sizeof(mh))
	{
		printf("Can't read header: %s\n", infile);
		exit(1);
	}
	if ((mh.type != MINIX_SPLITID_AHISTORICAL && mh.type != MINIX_SPLITID) ||
			(elks_size_t)mh.tseg == 0)
	{
		printf("Exec header not supported: %s\n", infile);
		exit(1);
	}

	if (mh.hlen == EXEC_FARTEXT_HDR_SIZE)
	{
		n = read(ifd, &eh, sizeof(eh));
		if (n != sizeof(mh))
		{
			printf("Can't read extended header: %s\n", infile);
			exit(1);
		}
	}
	else if (mh.hlen != EXEC_MINIX_HDR_SIZE)
	{
		printf("Header size not supported: %s\n", infile);
		exit(1);
	}
	else memset(&eh, 0, sizeof(eh));

	if (eh.esh_compr_tseg || eh.esh_compr_dseg || eh.esh_compr_ftseg)
	{
		printf("Binary already compressed: %s\n", infile);
		exit(1);
	}

	sztext = (elks_size_t)mh.tseg;
	szdata = (elks_size_t)mh.dseg;
	szftext = (elks_size_t)eh.esh_ftseg;
	printf("text %d ftext %d data %d\n", sztext, szftext, szdata);

	text = readsection(ifd, sztext);
	if (szftext)
		ftext = readsection(ifd, szftext);
	if (szdata)
		data = readsection(ifd, szdata);

	if (do_text)
	{
		/*
		 * compress text section -> ex.out
		 */
		sprintf(cmd, "%s raw -q -C -b %s,%d,%d -o %s",
			exomizer_binary, infile, (int)sizeof(mh), sztext, exomizer_outfile);
		printf("%s\n", cmd);
		unlink(exomizer_outfile);
		if (system(cmd) != 0)
		{
			printf("Text compression failed: %s\n", infile);
			exit(1);
		}
		if (stat(exomizer_outfile, &sbuf) < 0)
		{
			printf("Can't stat exomizer output: %s\n", exomizer_outfile);
			exit(1);
		}
		if (sbuf.st_size > sztext)
		{
			printf("Rejecting conversion of %s: compressed text larger than text\n", infile);
			exit(1);
		}
		if (sbuf.st_size >= 65520)
		{
			printf("Rejecting conversion of %s: compressed text too large (%ld)\n",
				infile, (long)sbuf.st_size);
			exit(1);
		}
		compr_sztext = (elks_size_t)sbuf.st_size;
		/* safety offset not yet checked: not available from external exomizer*/

		if ((efd = open(exomizer_outfile, O_RDONLY)) < 0)
		{
			printf("Can't open exomizer output: %s\n", exomizer_outfile);
			exit(1);
		}
		compr_text = readsection(efd, compr_sztext);
		eh.esh_compr_tseg = compr_sztext;
		close(efd);
		unlink(exomizer_outfile);
		printf("Text compressed from %d to %d\n", sztext, compr_sztext);
	}

	if (do_ftext && szftext)
	{
		/*
		 * compress fartext section -> ex.out
		 */
		sprintf(cmd, "%s raw -q -C -b %s,%d,%d -o %s",
			exomizer_binary, infile, (int)sizeof(mh)+sztext, szftext, exomizer_outfile);
		printf("%s\n", cmd);
		unlink(exomizer_outfile);
		if (system(cmd) != 0)
		{
			printf("Text compression failed: %s\n", infile);
			exit(1);
		}
		if (stat(exomizer_outfile, &sbuf) < 0)
		{
			printf("Can't stat exomizer output: %s\n", exomizer_outfile);
			exit(1);
		}
		if (sbuf.st_size > szftext)
		{
			printf("Rejecting conversion of %s: compressed fartext larger than fartext\n", infile);
			exit(1);
		}
		if (sbuf.st_size >= 65520)
		{
			printf("Rejecting conversion of %s: compressed fartext too large (%ld)\n",
				infile, (long)sbuf.st_size);
			exit(1);
		}
		compr_szftext = (elks_size_t)sbuf.st_size;
		/* safety offset not yet checked: not available from external exomizer*/

		if ((efd = open(exomizer_outfile, O_RDONLY)) < 0)
		{
			printf("Can't open exomizer output: %s\n", exomizer_outfile);
			exit(1);
		}
		compr_ftext = readsection(efd, compr_szftext);
		eh.esh_compr_ftseg = compr_szftext;
		close(efd);
		unlink(exomizer_outfile);
		printf("Far text compressed from %d to %d\n", szftext, compr_szftext);
	}

	if (do_data && szdata)
	{
		/*
		 * compress data section -> ex.out
		 */
		sprintf(cmd, "%s raw -q -C -b %s,%d,%d -o %s",
			exomizer_binary, infile, (int)sizeof(mh)+sztext+szftext, szdata, exomizer_outfile);
		printf("%s\n", cmd);
		unlink(exomizer_outfile);
		if (system(cmd) != 0)
		{
			printf("data compression failed: %s\n", infile);
			exit(1);
		}
		if (stat(exomizer_outfile, &sbuf) < 0)
		{
			printf("Can't stat exomizer output: %s\n", exomizer_outfile);
			exit(1);
		}
		if (sbuf.st_size > szdata)
		{
			printf("Rejecting conversion of %s: compressed data larger than data\n", infile);
			exit(1);
		}
		if (sbuf.st_size >= 65520)
		{
			printf("Rejecting conversion of %s: compressed data too large (%ld)\n",
				infile, (long)sbuf.st_size);
			exit(1);
		}
		compr_szdata = (elks_size_t)sbuf.st_size;
		/* safety offset not yet checked: not available from external exomizer*/

		if ((efd = open(exomizer_outfile, O_RDONLY)) < 0)
		{
			printf("Can't open exomizer output: %s\n", exomizer_outfile);
			exit(1);
		}
		compr_data = readsection(efd, compr_szdata);
		eh.esh_compr_dseg = compr_szdata;
		close(efd);
		unlink(exomizer_outfile);
		printf("Data compressed from %d to %d\n", szdata, compr_szdata);
	}

	if ((ofd = creat(outfile, 0755)) < 0) {
		printf("Can't create %s\n", outfile);
		exit(1);
	}

	mh.hlen = EXEC_FARTEXT_HDR_SIZE;
	writesection(ofd, &mh, sizeof(mh));
	writesection(ofd, &eh, sizeof(eh));
	if (do_text)
		writesection(ofd, compr_text, compr_sztext);
	else writesection(ofd, text, sztext);
	if (do_ftext && szftext)
		writesection(ofd, compr_ftext, compr_szftext);
	else writesection(ofd, ftext, szftext);
	if (do_data && szdata)
		writesection(ofd, compr_data, compr_szdata);
	else writesection(ofd, data, szdata);
	copyrest(ifd, ofd);
	close(ofd);
	close(ifd);

	stat(infile, &sbuf);
	orig_size = sbuf.st_size;
	stat(outfile, &sbuf);
	compr_size = sbuf.st_size;
	printf("%s: compressed from %ld to %ld (%ld%% reduction)\n",
		outfile, orig_size, compr_size, 100 - (compr_size*100/orig_size));
	exit(0);
}
