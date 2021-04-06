#include <stdio.h>
#include <stdlib.h>

void generate(const char *name, FILE *in, FILE *out)
{
    char buf[12];
    char *glue = "";
    int len;
    int size = 0;

    fprintf(out, "static unsigned char %s_arr[] = {\n", name);
    while((len = fread(buf, 1, 12, in)) > 0)
    {
        int col;

        size += len;
        fprintf(out, "    ");
        for(col = 0; col < len; ++col)
        {
            fprintf(out, "%s0x%02x", glue, (unsigned char)buf[col]);
            glue = ",";
        }
        fprintf(out, "\n");
    }
    fprintf(out, "};\n");
    fprintf(out, "struct buf %s = {%s_arr, %d, %d};\n",
            name, name, size, size);
}

int main(int argc, char *argv[])
{
    int i;
    if(argc < 2)
    {
        fprintf(stderr, "Error: must give at least one input file.\n");
    }

    fprintf(stdout, "#include \"buf.h\"\n");
    for(i = 1; i < argc; ++i)
    {
        FILE *in = fopen(argv[i], "rb");
        if(in == NULL)
        {
            fprintf(stderr, "Error: can't open file \"%s\" for input.\n",
                    argv[i]);
            exit(1);
        }
        generate(argv[i], in, stdout);
        fclose(in);
    }
    return 0;
}
