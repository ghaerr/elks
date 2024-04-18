/*
 * ELKS a.out executable compressor
 *
 * Apr 2021 Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <linuxmt/minix.h>

int keep_infile = 0;
int verbose = 0;
char exomizer_binary[] = "exomizer";

typedef unsigned short elks_size_t;

static char *readsection(int fd, int size, char *filename)
{
	char *p;

	if (size == 0)
		return NULL;

	if ((p = malloc(size)) == NULL)
	{
		printf("Out of memory\n");
		exit(1);
	}
	if (read(fd, p, size) != size)
	{
		printf("Error reading section: %s\n", filename);
		exit(1);
	}
	return p;
}

static void writesection(int fd, void *section, int size, char *filename)
{
	if (write(fd, section, size) != size)
	{
		printf("Write error: %s\n", filename);
		close(fd);
		unlink(filename);
		exit(1);
	}
}

static void copyrest(int ifd, int ofd, char *filename)
{
	int size;
	char buf[1024];

	while ((size = read(ifd, buf, sizeof(buf))) > 0)
	{
		if (write(ofd, buf, size) != size)
		{
error:
			printf("Write error: %s\n", filename);
			close(ofd);
			unlink(filename);
			exit(1);
		}
	}
	if (size != 0) goto error;
}

static int compress(char *infile, char *outfile, int do_text, int do_ftext, int do_data)
{
	int ifd, ofd, efd, n;
	long orig_size, compr_size;
	elks_size_t szhdr;
	elks_size_t sztext, szdata, szftext;
	elks_size_t compr_sztext, compr_szdata, compr_szftext;
	char *text, *data = NULL, *ftext = NULL;
	char *compr_text = NULL, *compr_data = NULL, *compr_ftext = NULL;
	struct stat sbuf;
	struct minix_exec_hdr mh;
	struct elks_supl_hdr eh;
	char *exomizer_outfile = "ex.out";
	char cmd[256];

	if ((ifd = open(infile, O_RDONLY)) < 0)
	{
		printf("Can't open %s\n", infile);
		return 1;
	}

	n = read(ifd, &mh, sizeof(mh));
	if (n != sizeof(mh))
	{
		printf("Can't read header: %s\n", infile);
		return 1;
	}
	if ((mh.type != MINIX_SPLITID_AHISTORICAL && mh.type != MINIX_SPLITID) ||
			(elks_size_t)mh.tseg == 0)
	{
		printf("Exec header not supported: %s\n", infile);
		return 2;
	}

	if (mh.hlen == EXEC_FARTEXT_HDR_SIZE)
	{
		n = read(ifd, &eh, sizeof(eh));
		if (n != sizeof(mh))
		{
			printf("Can't read extended header: %s\n", infile);
			return 1;
		}
	}
	else if (mh.hlen != EXEC_MINIX_HDR_SIZE)
	{
		printf("Header size not supported: %s\n", infile);
		return 1;
	}
	else memset(&eh, 0, sizeof(eh));
	szhdr = mh.hlen;

	if (eh.esh_compr_tseg || eh.esh_compr_dseg || eh.esh_compr_ftseg)
	{
		printf("Binary already compressed: %s\n", infile);
		return 2;
	}

	sztext = (elks_size_t)mh.tseg;
	szdata = (elks_size_t)mh.dseg;
	szftext = (elks_size_t)eh.esh_ftseg;
	if (verbose) printf("text %d ftext %d data %d\n", sztext, szftext, szdata);

	text = readsection(ifd, sztext, infile);
	if (szftext)
		ftext = readsection(ifd, szftext, infile);
	if (szdata)
		data = readsection(ifd, szdata, infile);

	if (do_text)
	{
		/*
		 * compress text section -> ex.out
		 */
		sprintf(cmd, "%s raw -q -C -b %s,%d,%d -o %s",
			exomizer_binary, infile, szhdr, sztext, exomizer_outfile);
		if (verbose) printf("%s\n", cmd);
		unlink(exomizer_outfile);
		if (system(cmd) != 0)
		{
			printf("Text compression failed: %s\n", infile);
			return 1;
		}
		if (stat(exomizer_outfile, &sbuf) < 0)
		{
			printf("Can't stat exomizer output: %s\n", exomizer_outfile);
			return 1;
		}
		if (sbuf.st_size > sztext)
		{
			printf("Rejecting conversion of %s: compressed text larger than text\n", infile);
			unlink(exomizer_outfile);
			return 2;
		}
		if (sbuf.st_size >= 65520)
		{
			printf("Rejecting conversion of %s: compressed text too large (%ld)\n",
				infile, (long)sbuf.st_size);
			unlink(exomizer_outfile);
			return 2;
		}
		compr_sztext = (elks_size_t)sbuf.st_size;
		/* safety offset not yet checked: not available from external exomizer*/

		if ((efd = open(exomizer_outfile, O_RDONLY)) < 0)
		{
			printf("Can't open exomizer output: %s\n", exomizer_outfile);
			return 1;
		}
		compr_text = readsection(efd, compr_sztext, exomizer_outfile);
		eh.esh_compr_tseg = compr_sztext;
		close(efd);
		unlink(exomizer_outfile);
		if (verbose) printf("compressed text from %d to %d\n", sztext, compr_sztext);
	}

	if (do_ftext && szftext)
	{
		/*
		 * compress fartext section -> ex.out
		 */
		sprintf(cmd, "%s raw -q -C -b %s,%d,%d -o %s",
			exomizer_binary, infile, szhdr+sztext, szftext, exomizer_outfile);
		if (verbose) printf("%s\n", cmd);
		unlink(exomizer_outfile);
		if (system(cmd) != 0)
		{
			printf("Text compression failed: %s\n", infile);
			return 1;
		}
		if (stat(exomizer_outfile, &sbuf) < 0)
		{
			printf("Can't stat exomizer output: %s\n", exomizer_outfile);
			return 1;
		}
		if (sbuf.st_size > szftext)
		{
			printf("Rejecting conversion of %s: compressed fartext larger than fartext\n", infile);
			unlink(exomizer_outfile);
			return 2;
		}
		if (sbuf.st_size >= 65520)
		{
			printf("Rejecting conversion of %s: compressed fartext too large (%ld)\n",
				infile, (long)sbuf.st_size);
			unlink(exomizer_outfile);
			return 2;
		}
		compr_szftext = (elks_size_t)sbuf.st_size;
		/* safety offset not yet checked: not available from external exomizer*/

		if ((efd = open(exomizer_outfile, O_RDONLY)) < 0)
		{
			printf("Can't open exomizer output: %s\n", exomizer_outfile);
			return 1;
		}
		compr_ftext = readsection(efd, compr_szftext, exomizer_outfile);
		eh.esh_compr_ftseg = compr_szftext;
		close(efd);
		unlink(exomizer_outfile);
		if (verbose) printf("compressed fartext from %d to %d\n", szftext, compr_szftext);
	}

	if (do_data && szdata)
	{
		/*
		 * compress data section -> ex.out
		 */
		sprintf(cmd, "%s raw -q -C -b %s,%d,%d -o %s",
			exomizer_binary, infile, szhdr+sztext+szftext, szdata, exomizer_outfile);
		if (verbose) printf("%s\n", cmd);
		unlink(exomizer_outfile);
		if (system(cmd) != 0)
		{
			printf("data compression failed: %s\n", infile);
			return 1;
		}
		if (stat(exomizer_outfile, &sbuf) < 0)
		{
			printf("Can't stat exomizer output: %s\n", exomizer_outfile);
			return 1;
		}
		if (sbuf.st_size > szdata)
		{
			printf("Rejecting data conversion of %s: compressed data larger than data\n", infile);
			unlink(exomizer_outfile);
			do_data = 0;
			goto next;
		}
		if (sbuf.st_size >= 65520)
		{
			printf("Rejecting conversion of %s: compressed data too large (%ld)\n",
				infile, (long)sbuf.st_size);
			unlink(exomizer_outfile);
			return 2;
		}
		compr_szdata = (elks_size_t)sbuf.st_size;
		/* safety offset not yet checked: not available from external exomizer*/

		if ((efd = open(exomizer_outfile, O_RDONLY)) < 0)
		{
			printf("Can't open exomizer output: %s\n", exomizer_outfile);
			return 1;
		}
		compr_data = readsection(efd, compr_szdata, exomizer_outfile);
		eh.esh_compr_dseg = compr_szdata;
		close(efd);
		unlink(exomizer_outfile);
		if (verbose) printf("compressed data from %d to %d\n", szdata, compr_szdata);
	}

next:
	if ((ofd = creat(outfile, 0755)) < 0) {
		printf("Can't create %s\n", outfile);
		close(ifd);
		return 1;
	}

	mh.hlen = EXEC_FARTEXT_HDR_SIZE;
	writesection(ofd, &mh, sizeof(mh), outfile);
	writesection(ofd, &eh, sizeof(eh), outfile);
	if (do_text)
		writesection(ofd, compr_text, compr_sztext, outfile);
	else writesection(ofd, text, sztext, outfile);
	if (do_ftext && szftext)
		writesection(ofd, compr_ftext, compr_szftext, outfile);
	else writesection(ofd, ftext, szftext, outfile);
	if (do_data && szdata)
		writesection(ofd, compr_data, compr_szdata, outfile);
	else writesection(ofd, data, szdata, outfile);
	copyrest(ifd, ofd, outfile);
	close(ofd);
	close(ifd);

	if (text)			free(text);
	if (ftext)			free(ftext);
	if (data)			free(data);
	if (compr_text)		free(compr_text);
	if (compr_ftext)	free(compr_ftext);
	if (compr_data)		free(compr_data);

	stat(infile, &sbuf);
	orig_size = sbuf.st_size;
	stat(outfile, &sbuf);
	compr_size = sbuf.st_size;
	printf("%s: compressed from %ld to %ld (%ld%% reduction)\n",
		infile, orig_size, compr_size, 100 - (compr_size*100/orig_size));

	return 0;
}

static int issymlink(char *filename)
{
	struct stat sbuf;

	if (lstat(filename, &sbuf) < 0)
		return 0;
	return ((sbuf.st_mode & S_IFMT) == S_IFLNK);
}

#if DOHARDLINKS
static int hardlinks(char *filename)
{
	struct stat sbuf;

	if (stat(filename, &sbuf) < 0)
		return 0;
	return (sbuf.st_nlink > 1);
}

int copyfile(char *srcname, char *destname)
{
	int rfd, wfd, rcc;
	char buf[1024];

	rfd = open(srcname, O_RDONLY);
	if (rfd < 0)
	{
		perror(srcname);
		return 1;
	}

	wfd = open(destname, O_WRONLY | O_TRUNC);
	if (wfd < 0)
	{
		perror(destname);
		close(rfd);
		return 1;
	}

	while ((rcc = read(rfd, buf, sizeof(buf))) > 0)
	{
		if (write(wfd, buf, rcc) != rcc)
		{
			perror(destname);
			goto error_exit;
		}
	}

	if (rcc < 0)
	{
		perror(srcname);
		goto error_exit;
	}

	close(rfd);
	close(wfd);
	return 0;

error_exit:
	close(rfd);
	close(wfd);
	return 1;
}
#endif

static void usage(void)
{
	printf("Usage: elks-compress [-vztfd] [-o outfile] file [...]\n");
	printf("	-v: verbose\n"
	       "	-z: keep input file (create input.z)\n"
		   "	-t: compress just text section\n"
		   "	-f: compress just fartext section\n"
		   "	-d: compress just data section\n");
	exit(1);
}

int main(int ac, char **av)
{
	int ret, exitval;
	char *outfile = NULL;
	int do_text = 0;
	int do_ftext = 0;
	int do_data = 0;
	char outname[256];

	while ((ret = getopt(ac, av, "ztfdo:")) != -1)
	{
		switch (ret)
		{
		case 'v':
			verbose = 1;
			break;
		case 'z':
			keep_infile = 1;
			break;
		case 't':
			do_text = 1;
			break;
		case 'f':
			do_ftext = 1;
			break;
		case 'd':
			do_data = 1;
			break;
		case 'o':
			outfile = optarg;
			break;
		default:
			usage();
		}
	}

	if (optind >= ac)
		usage();

	/* allow only one input file with -o */
	if ((ac - optind > 1) && outfile)
		usage();

	if (outfile)
		keep_infile = 1;
	if (!do_text && !do_ftext && !do_data)
		do_text = do_ftext = do_data = 1;

	exitval = 0;
	while (optind < ac)
	{
		if (issymlink(av[optind]))
		{
			fprintf(stderr, "Ignoring symlink %s\n", av[optind]);
			ac--;
			av++;
			continue;
		}

		if (outfile == NULL)
		{
			if (keep_infile)
			{
				sprintf(outname, "%s.z", av[optind]);
			}
			else
			{
				sprintf(outname, "ec.out");
			}
		}
		ret = compress(av[optind], outfile? outfile: outname, do_text, do_ftext, do_data);
		if (ret == 0)
		{
			if (!keep_infile)
			{
#if DOHARDLINKS
				if (!outfile && hardlinks(av[optind]))
				{
					if (copyfile(outname, av[optind]))
						exitval = 1;
				}
				else
#endif
				{
					if (unlink(av[optind]))
					{
						perror("unlink");
						exitval = 1;
					}
					else if (rename(outname, av[optind]))
					{
						perror("rename");
						exitval = 1;
					}
				}
			}
		} else {
			unlink(outfile? outfile: outname);
			if (ret != 2)		/* Ignore "Rejecting" errors */
				exitval = 1;
		}

		ac--;
		av++;
	}
	return exitval;
}
