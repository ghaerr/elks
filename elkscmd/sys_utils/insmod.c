#include <stdio.h>
/* #include <unistd.h> */
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <linuxmt/config.h>
#include <linuxmt/mem.h>

char *failed = "insmod: failed\n";
#define	F_LEN	15
#define STDERR_FILENO 2

static char modbuf[DATASIZE < TEXTSIZE ? TEXTSIZE : DATASIZE];

int main(argc,argv)
int argc;
char **argv;
{
	int	od,md,i;
	char	fname[20];
	unsigned int	data, text, len, doff, coff;
	off_t	datap, textp;
	struct stat	buf;

	if (argc != 2) {
		write(STDERR_FILENO, "usage: insmod <module>\n",23);
		exit(1);
	}
	/* Clean up the old module */
	insmod(MOD_TERM);
	/* Open the mem device to access kernel space */
	if ((md = open("/dev/mem",O_WRONLY)) == -1) {
		write(STDERR_FILENO, failed, F_LEN);
		write(STDERR_FILENO, "Could not open /dev/mem.\n",25);
		exit(errno);
	}
	/* Get the addresses in kernel space to copy the module to */
	if (((ioctl(md, MEM_GETMODTEXT, &text)) == -1) ||
	    ((ioctl(md, MEM_GETMODDATA, &data)) == -1) ||
	    ((ioctl(md, MEM_GETDS, &doff)) == -1) ||
	    ((ioctl(md, MEM_GETCS, &coff)) == -1)) {
		write(STDERR_FILENO, failed, F_LEN);
		write(STDERR_FILENO, "Kernel does not support modules.\n",33);
		exit(errno);
	}
	/* Calculate /dev/mem seek points */
	datap = ((long)doff << 4) + (long)data;
	textp = ((long)coff << 4) + (long)text;
	/* Open <module>.T */
	strcpy(fname, argv[1], 15);
	strcat(fname, ".T");
	if ((od = open(fname,O_RDONLY)) == -1) {
		write(STDERR_FILENO, failed, F_LEN);
		write(STDERR_FILENO, "Could not open module ",22);
		write(STDERR_FILENO, argv[1], strlen(argv[1]));
		exit(errno);
	}
	if ((fstat(od, &buf)) == -1) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	if ((len = buf.st_size) > TEXTSIZE) {
		write(STDERR_FILENO, failed, F_LEN);
		write(STDERR_FILENO, "Module too big.\n",16);
		exit(errno);
	}
	if ((lseek(md, textp, SEEK_SET)) == -1) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	/* Copy <module>.T into the kernel */
	if ((read(od, modbuf, len)) != len) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	if ((write(md, modbuf, len)) != len) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	/* Open <module>.D */
	strcpy(fname, argv[1], 15);
	strcat(fname, ".D");
	if ((od = open(fname,O_RDONLY)) == -1) {
		write(STDERR_FILENO, failed, F_LEN);
		write(STDERR_FILENO, "Could not open module ",22);
		write(STDERR_FILENO, argv[1], strlen(argv[1]));
		exit(errno);
	}
	if ((fstat(od, &buf)) == -1) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	if ((len = buf.st_size) > TEXTSIZE) {
		write(STDERR_FILENO, failed, F_LEN);
		write(STDERR_FILENO, "Module too big.\n",16);
		exit(errno);
	}
	if ((lseek(md, datap, SEEK_SET)) == -1) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	/* Copy <module>.D into the kernel */
	if ((read(od, modbuf, len)) != len) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	if ((write(md, modbuf, len)) != len) {
		write(STDERR_FILENO, failed, F_LEN);
		exit(errno);
	}
	fflush(stdout);
	/* Initialise the module. */
	insmod(MOD_INIT);
}
