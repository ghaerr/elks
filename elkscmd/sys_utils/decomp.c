/*
 * decomp - decompress compressed ELKS executables
 *
 * Apr 2024 Greg Haerr
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <linuxmt/minix.h>

static struct minix_exec_hdr mh;
static struct elks_supl_hdr  eh;

static int decomp_section(int ifd, int ofd, unsigned int outsz, unsigned int insz)
{
    char *buf;

    if (insz == 0)              /* nothing to decompress */
        return 0;
    /*printf("Decompressing %u bytes to %u\n", insz, outsz);*/
    buf = malloc(outsz+16);     /* will fail above 32K on ELKS */
    if (!buf) {
        fprintf(stderr, "Memory alloc fail: %u bytes\n", outsz+16);
        return -1;
    }
    if (read(ifd, buf, insz) != insz) {
        perror("read");
        return -1;
    }
    if (!decompress(buf, 0, outsz, insz, 16))
        return -1;
    if (write(ofd, buf, outsz) != outsz) {
        perror("write");
        return -1;
    }
    free(buf);
    return 0;
}

static int decomp_file(char *path, char *outfile)
{
    int ifd, ofd;
    int needs_ehdr, needs_tmpfile;

    if ((ifd = open(path, O_RDONLY)) < 0) {
        perror(path);
        return -1;
    }
    if (read(ifd, &mh, sizeof(mh)) != sizeof(mh) ||
        read(ifd, &eh, sizeof(eh)) != sizeof(eh)) {
            perror("read");
            return -1;
    }
    if ((mh.type != MINIX_SPLITID_AHISTORICAL && mh.type != MINIX_SPLITID) || !mh.tseg) {
        fprintf(stderr, "%s: not an executable\n", path);
        return -1;
    }
    if ((mh.hlen != sizeof(mh) + sizeof(eh)) ||
        (!eh.esh_compr_tseg && !eh.esh_compr_ftseg && !eh.esh_compr_dseg)) {
            fprintf(stderr, "%s: not compressed\n", path);
            return 0;
    }
    needs_ehdr = (uint16_t)eh.esh_ftseg || eh.msh_trsize || eh.msh_drsize;

    /*printf(" text: %5u %5u\n", (uint16_t)mh.tseg, eh.esh_compr_tseg);
    printf(" data: %5u %5u\n", (uint16_t)mh.dseg, eh.esh_compr_dseg);
    printf("ftext: %5u %5u\n", (uint16_t)eh.esh_ftseg, eh.esh_compr_ftseg);
    printf(" size: %5u %5u\n",
        (uint16_t)mh.tseg + (uint16_t)mh.dseg + (uint16_t)eh.esh_ftseg,
        eh.esh_compr_tseg + eh.esh_compr_dseg + eh.esh_compr_ftseg);*/
    printf("%u -> %u bytes\n",
        eh.esh_compr_tseg + eh.esh_compr_dseg + eh.esh_compr_ftseg,
        (uint16_t)mh.tseg + (uint16_t)mh.dseg + (uint16_t)eh.esh_ftseg);

    needs_tmpfile = !outfile;
    if (needs_tmpfile)
        outfile = "decomp.tmp";
    unlink(outfile);
    if ((ofd = creat(outfile, 0755)) < 0) {
        perror(outfile);
        return -1;
    }
    if (write(ofd, &mh, sizeof(mh)) != sizeof(mh) ||
        (needs_ehdr && (write(ofd, &eh, sizeof(eh)) != sizeof(eh)))) {
            perror("write");
            return -1;
    }
    if (decomp_section(ifd, ofd, (uint16_t)mh.tseg, eh.esh_compr_tseg) < 0 ||
        decomp_section(ifd, ofd, (uint16_t)eh.esh_ftseg, eh.esh_compr_ftseg) < 0 ||
        decomp_section(ifd, ofd, (uint16_t)mh.dseg, eh.esh_compr_dseg) < 0) {
            return -1;
    }
    if (lseek(ofd, 0L, SEEK_SET) != 0) {
        perror("lseek");
        return -1;
    }
    if (needs_ehdr) {
        eh.esh_compr_tseg = 0;
        eh.esh_compr_dseg = 0;
        eh.esh_compr_ftseg = 0;
    } else {
        mh.hlen = sizeof(struct minix_exec_hdr);
    }
    if (write(ofd, &mh, sizeof(mh)) != sizeof(mh) ||
        (needs_ehdr && (write(ofd, &eh, sizeof(eh)) != sizeof(eh)))) {
            perror("write");
            return -1;
    }
    close(ifd);
    close(ofd);
    if (needs_tmpfile) {
        unlink(path);
        if (rename(outfile, path) < 0) {
            fprintf(stderr, "Rename failed: %s to %s\n", outfile, path);
            return -1;
        }
    }
    return 0;
}

static int usage(void)
{
    fprintf(stderr, "usage: decomp <program> [<decompressed_program>]\n");
    exit(1);
}

int main(int ac, char **av)
{
    char *outfile = NULL;

    if (ac < 2 || ac > 3)
        usage();
    if (ac == 3)
        outfile = av[2];
    return decomp_file(av[1], outfile);
}
