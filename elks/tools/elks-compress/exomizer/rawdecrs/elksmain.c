
#include "exodecr.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define BUF_SIZE	(65536 * 10)
int decompress(char *buf, int seg, int orig_size, int compr_size, int safety);

int main(int argc, char *argv[])
{
    FILE *in;
    FILE *out;
    char *inp;
    char *outp;
	char *newoutp;
    char *p;
    int len;
	int origsize, safety;
	int size;
	struct stat sbuf;

    if(argc != 5)
    {
        fprintf(stderr, "Error: usage: %s <infile> <outfile> <orig_size> <safety_offset>\n", argv[0]);
        exit(-1);
    }
    in = fopen(argv[1], "rb");
    if(in == NULL)
    {
        fprintf(stderr, "Error: can't open %s for input.\n", argv[1]);
        exit(-1);
    }
    out = fopen(argv[2], "wb");
    if(in == NULL)
    {
        fprintf(stderr, "Error: can't open %s for output.\n", argv[2]);
        exit(-1);
    }
	origsize = atoi(argv[3]);
	safety = atoi(argv[4]);

    p = malloc(BUF_SIZE);
	if (fstat(fileno(in), &sbuf) < 0) {
		perror("STAT");
		return 1;
	}
	len = (int)sbuf.st_size;
	printf("ORIG %d compress %d safety %d extra %d\n",
		origsize, len, safety, origsize-len+safety);

	safety = 16;
#if 0
	inp = p + BUF_SIZE - len - (origsize - len + safety);
    if (fread(inp, 1, BUF_SIZE, in) != len) {
		perror("FREAD");
		return 1;
	}
	inp += len;
    outp = p + BUF_SIZE;
	outp = inp + len + (origsize - len + safety);

    newoutp = exo_decrunch(inp+len, outp);
    fwrite(newoutp, 1, outp-newoutp, out);
	printf("WROTE %d\n", outp-newoutp);
#else
	inp = p;
    if (fread(inp, 1, BUF_SIZE, in) != len) {
		perror("FREAD");
		return 1;
	}
	outp = inp + origsize + safety;

    size = decompress(p, 0, origsize, len, safety);
    fwrite(p, 1, size, out);
	printf("WROTE %d\n", size);
#endif

    free(p);

    fclose(out);
    fclose(in);

    return 0;
}
