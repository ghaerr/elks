/* 
 * file:         passwd.c
 * description:  change user password
 * author:       Shane Kerr <kerr@wizard.net>
 * copyright:    1998 by Shane Kerr <kerr@wizard.net>, under terms of GPL
 */

/* todo:  make executable setuid on execute */
/*        needs to check setuid bit in fs/exec.c of ELKS kernel */
/* todo:  add a set of rules to make sure trivial passwords are not used */
/* todo:  lock password file while updating */
/*        need file locking first */
/* todo:  use rename to do an atomic move of temporary file */
/*        need rename that overwrites first */

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* maximum errors */
#define MAX_FAILURE 5

/* valid characters for a salt value */
char salt_char[] = 
          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

int main(argc,argv)
int argc;
char ** argv;
{
    struct passwd *pwd;
    char *pbuf, salt[3];
    char nbuf1[128], nbuf2[128];
    int n;
    time_t now;
    int ch;
    int failure_cnt;
    FILE *passwd_fp;
    char *tmp_fname;
    int uid;

    switch (argc) {
        case 1:
	    pwd = getpwuid(getuid());
	    if (pwd == NULL) {
		fprintf(stderr, "%s: Error reading password file\n", argv[0]);
	        return 1;
	    }
	    break;
        case 2:
	    pwd = getpwnam(argv[1]);
  	    if (pwd == NULL) {
		fprintf(stderr, "%s: Unknown user %s\n", argv[0], argv[1]);
	        return 1;
	    }
	    if ((getuid() != 0) && (getuid() != pwd->pw_uid)) {
		fprintf(stderr, "You may not change the password for %s.\n", 
		     argv[1]);
		return 1;
	    }
	    break;
        default:
	    fprintf(stderr, "usage: %s [ name ]\n", argv[0]);
	    return 1;
    }

    printf("Changing password for %s\n", pwd->pw_name);

    if ((getuid() != 0) && (pwd->pw_passwd[0] != 0)) {
	pbuf = getpass("Old password:");
	putchar('\n');
	salt[0] = pwd->pw_passwd[0];
	salt[1] = pwd->pw_passwd[1];
	salt[2] = 0;
	if (strcmp(crypt(pbuf,salt),pwd->pw_passwd)) {
	    fprintf(stderr, "Incorrect password for %s.\n", pwd->pw_name);
	    return 1;
	}
	memset(pbuf, 0, strlen(pbuf));
    }

    failure_cnt = 0;
    for (;;) {
        pbuf = getpass("New password:");
	putchar('\n');
	strcpy(nbuf1, pbuf);
	memset(pbuf, 0, strlen(pbuf));
	pbuf = getpass("Re-enter new password:");
	putchar('\n');
	strcpy(nbuf2, pbuf);
	memset(pbuf, 0, strlen(pbuf));
	if (strcmp(nbuf1, nbuf2) == 0) {
	  /* I'm just going to use the LSB of time for salt */
	    time(&now);
	    ch = salt_char[now & 0x3F];
	    salt[0] = ch;
	    ch = salt_char[(now >> 6) & 0x3F];
	    salt[1] = ch;
	    salt[2] = 0;

	  /* need to create a temporary file for the new password */
	    tmp_fname = tmpnam(NULL);
	    if (tmp_fname == NULL) {
		perror("Error getting temporary file name");
		return 1;
	    }
	    passwd_fp = fopen(tmp_fname, "w");
	    if (passwd_fp == NULL) {
		fprintf(stderr, "Error creating temporary password file\n");
		return 1;
	    }

          /* save off our UID */
	    uid = pwd->pw_uid;

	  /* save entries to the new file */
	    pwd = getpwent();
	    while (pwd != NULL) {
		if (pwd->pw_uid == uid) {
		    pwd->pw_passwd = crypt(nbuf1, salt);
		}
	        if (putpwent(pwd, passwd_fp) == -1) {
		    perror("Error writing password file");
		    return 1;
	        }
	        pwd = getpwent();
	    }

	  /* update the file and move it into position */
	    fclose(passwd_fp);
	    if (unlink("/etc/passwd") == -1) {
		perror("Error removing old password file");
		return 1;
	    }
	    if (rename(tmp_fname, "/etc/passwd") == -1) {
		perror("Error installing new password file");
		return 1;
	    }

	  /* celebrate our success */
	    printf("Password changed.\n");
	    return 0;
	}
	if (++failure_cnt >= MAX_FAILURE) {
	    fprintf(stderr, "The password for %s is unchanged.\n",pwd->pw_name);
            return 1;
	}
	fprintf(stderr, "They don't match; try again.\n");
    }
}

