
#include "exodecr.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *in;
    FILE *out;
    char *inp;
    char *outp;
    char *p;
    int len;

    if(argc != 3)
    {
        fprintf(stderr, "Error: usage: %s <infile> <outfile>\n", argv[0]);
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

    p = malloc(500000);
    len = fread(p, 1, 500000, in);
    inp = p + len;
    outp = p + 500000;

    outp = exo_decrunch(inp, outp);

    fwrite(outp, 1, p + 500000 - outp, out);

    free(p);

    fclose(out);
    fclose(in);

    return 0;
}
