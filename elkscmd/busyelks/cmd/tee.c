/* tee - pipe fitting			Author: Paul Polderman */

#include "../sash.h"

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include "../defs.h"

#define	MAXFD	18

int tee_main(int argc, char **argv)
{
  char iflag = 0, aflag = 0;
  char buf[BUF_SIZE];
  int i, s, n;
  int fd[MAXFD];


  argv++;
  --argc;
  while (argc > 0 && argv[0][0] == '-') {
	switch (argv[0][1]) {
	    case 'i':		/* Interrupt turned off. */
		iflag++;
		break;
	    case 'a':		/* Append to outputfile(s), instead of
			 * overwriting them. */
		aflag++;
		break;
	    default:
		fprintf(stderr,"Usage: tee [-i] [-a] [files].\n");
		exit(1);
	}
	argv++;
	--argc;
  }
  fd[0] = 1;			/* Always output to stdout. */
  for (s = 1; s < MAXFD && argc > 0; --argc, argv++, s++) {
	if (aflag && (fd[s] = open(*argv, O_RDWR)) >= 0) {
		lseek(fd[s], 0L, SEEK_END);
		continue;
	} else {
		if ((fd[s] = creat(*argv, 0666)) >= 0) continue;
	}
	fprintf(stderr,"Cannot open output file: ");
	fprintf(stderr,*argv);
	fprintf(stderr,"\n");
	exit(2);
  }

  if (iflag) signal(SIGINT, SIG_IGN);

  while ((n = read(0, buf, BUF_SIZE)) > 0) {
	for (i = 0; i < s; i++) write(fd[i], buf, n);
  }

  for (i = 0; i < s; i++)	/* Close all fd's */
	close(fd[i]);
  return(0);
}
