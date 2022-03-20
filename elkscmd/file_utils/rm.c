#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <fcntl.h>

static 	int	errcode;

int usage(void)
{
	fprintf(stderr, "usage: rm [-rfi] file [...]\n");
	return 1;
}

int yes(void) {
        int i, b;

        i = b = getchar();
        while(b != '\n' && b != EOF)
                b = getchar();
        return(i == 'y');
}

int dotname(char *s) {
        if ((s[0] == '.')) {
                if (s[1] == '.')
                        if (s[2] == '\0')
                                return(1);
                        else
                                return(0);
                else if(s[1] == '\0')
                        return(1);
        }
	return(0);
}

void rm(char *arg, int fflg, int rflg, int iflg, int level) {
        struct stat buf;
        struct dirent *dp;
        DIR *dirp;
        char name[128];

        if(lstat(arg, &buf)) {
                if (fflg==0) {
                        printf("rm: %s nonexistent\n", arg);
                        ++errcode;
                }
                return;
        }
        if ((buf.st_mode&S_IFMT) == S_IFDIR) {
                if(rflg) {
                        if (access(arg, O_WRONLY) < 0) {
                                if (fflg==0)
                                        printf("rm: %s not changed\n", arg);
                                errcode++;
                                return;
                        }
                        if(iflg && level!=0) {
                                printf("rm: remove directory %s? ", arg);
                                if(!yes())
                                        return;
                        }
                        if((dirp = opendir(arg)) == NULL) {
                                printf("rm: cannot read %s?\n", arg);
                                return;
                        }
                        while((dp = readdir(dirp)) != NULL) {
                                if(dp->d_ino != 0 && !dotname(dp->d_name)) {
                                        sprintf(name, "%s/%s", arg, dp->d_name);
                                        rm(name, fflg, rflg, iflg, level+1);
                                }
                        }
                        closedir(dirp);
                        if (dotname(arg))
                                return;
                        if (rmdir(arg) < 0) {
                                fprintf(stderr, "rm: ");
                                perror(arg);
                                errcode++;
                        }
                        return;
                }
                fprintf(stderr, "rm: %s is a directory\n", arg);
                ++errcode;
                return;
        }

        if(iflg) {
                printf("rm: remove %s? ", arg);
                if(!yes())
                        return;
        } else if(!fflg) {
                if ((buf.st_mode&S_IFMT) != S_IFLNK && access(arg, 02) < 0) {
                        printf("rm: override protection %o for %s? ", buf.st_mode&0777, arg);
                        if(!yes())
                                return;
                }
        }
        if (unlink(arg) && (fflg==0 || iflg)) {
                fprintf(stderr, "rm: %s not removed\n", arg);
                ++errcode;
        }
	return;
}

int main(int argc, char **argv) {
	int force=0, recurse = 0, interact =0 ;
	char *arg;

	if (argc < 2)
		return usage();
	errcode = 0;

        while(argc>1 && argv[1][0]=='-') {
                arg = *++argv;
                argc--;

                /*
                 *  all files following a single '-' are considered file names
                 */
                if (*(arg+1) == '\0') break;

                while(*++arg != '\0')
                        switch(*arg) {
                        case 'f':
                                force++;
                                break;
                        case 'i':
                                interact++;
                                break;
                        case 'r':
                                recurse++;
                                break;
                        default:
                                return usage();
                        }
        }
        while(--argc > 0) {
                if(!strcmp(*++argv, "..")) {
                        fprintf(stderr, "rm: cannot remove `..'\n");
                        continue;
                }
                rm(*argv, force, recurse, interact, 0);
        }
        return(errcode);
}
