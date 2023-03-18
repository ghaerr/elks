/*
 * This implements the autocompletion callback (or TAB completion) for ash/linenoise.
 * Autocompletion operates identically to BASH shell. Greg Haerr 17 Apr 2020
 */

#include "shell.h"

#if LINENOISE		/* entire file if LINENOISE=0 in shell.h*/
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "linenoise.h"
#include "output.h"

void completion(const char *buf, linenoiseCompletions *lc)
{
	char *arg, *file;
	DIR *fp;
	struct dirent *dp;
	int count;
	char dir[PATH_MAX];  /* larger than NAME_MAX since this is user-supplied */
	char line[LINENOISE_MAX_LINE];
	char result[LINENOISE_MAX_LINE];
	char lastentry[LINENOISE_MAX_LINE];

	strcpy(line, buf);

	/* get active argument*/
	arg = rindex(line, ' ');
	if (arg)
		++arg;
	else arg = line;

	/* parse directory and filename portion*/
	file = rindex(arg, '/');
	if (file) {
		char c = *++file;
		*file = 0;
		strcpy(dir, arg);
		*file = c;
	}
	else {
		file = arg;
		strcpy(dir, (arg == line)? "/bin": ".");
	}

	fp = opendir(dir);
	if (!fp)
		return;

	/* first pass, count matches*/
	count = 0;
	while ((dp = readdir(fp)) != NULL) {
		if (!strncmp(file, dp->d_name, strlen(file))) {
			if ((file[0] == '.') || (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))) {
				strcpy(result, buf);
				strcpy(&result[strlen(result)-strlen(file)], dp->d_name);

				strcpy(lastentry, dp->d_name);
				count++;
			}
		}
	}
	closedir(fp);

	if (count == 0)
		return;
	if (count == 1) {
		struct stat st;
		char path[PATH_MAX];

		fmtstr(path, sizeof(path), "%s/%s", dir, lastentry);
		if (!stat(path, &st)) {
			if (S_ISDIR(st.st_mode))
				strcat(result, "/");
			if (S_ISREG(st.st_mode))
				strcat(result, " ");
		}
		linenoiseAddCompletion(lc, result);
		return;
	}

	/* display matches if more than one*/
	linenoiseAddCompletion(lc, buf);
	count = 0;
	fp = opendir(dir);
	while ((dp = readdir(fp)) != NULL) {
		if (!strncmp(file, dp->d_name, strlen(file))) {
			struct stat st;
			char path[PATH_MAX];

			if ((file[0] == '.') || (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))) {
				strcpy(result, buf);
				strcpy(&result[strlen(result)-strlen(file)], dp->d_name);
				linenoiseAddCompletion(lc, result);

				strcpy(result, dp->d_name);
				fmtstr(path, sizeof(path), "%s/%s", dir, dp->d_name);
				if (!stat(path, &st) && S_ISDIR(st.st_mode))
					strcat(result, "/");

				if ((count & 3) == 0)
					out1str("\r\n");
				outfmt(out1, "%-16s", result);
				count++;
			}
		}
	}
	if (count)
		out1str("\r\n");
	flushout(out1);
	closedir(fp);
}

#endif /* LINENOISE entire file*/
