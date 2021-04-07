
#include "exodecrunch.h"
#include <stdio.h>
#include <stdlib.h>

static int read_byte(void *read_data)
{
    FILE *in;
    int c;

    in = (FILE*)read_data;
    c = fgetc(in);

    return c;
}

int main(int argc, char *argv[])
{
    FILE *in;
    FILE *out;
    struct exo_decrunch_ctx *ctx;
    int c;

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

    ctx = exo_decrunch_new(MAX_OFFSET, read_byte, in);

    while((c = exo_read_decrunched_byte(ctx)) != EOF)
    {
        fputc(c, out);
    }

    exo_decrunch_delete(ctx);

    fclose(out);
    fclose(in);

    return 0;
}
