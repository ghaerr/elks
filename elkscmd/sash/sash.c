/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * Stand-alone shell for system maintainance for Linux.
 * This program should NOT be built using shared libraries.
 */

#include "sash.h"

#include <signal.h>
#include <errno.h>

typedef struct {
	char	*name;
	char	*usage;
	void	(*func)();
	int	minargs;
	int	maxargs;
} CMDTAB;


static	CMDTAB	cmdtab[] = {
#ifdef CMD_ALIAS
	"alias",	"[name [command]]", 	do_alias,
	1,		MAXARGS,
#endif

	"cd",		"[dirname]",		do_cd,
	1,		2,

#ifdef CMD_CHGRP
	"chgrp",	"gid filename ...",	do_chgrp,
	3,		MAXARGS,
#endif

#ifdef CMD_CHMOD
	"chmod",	"mode filename ...",	do_chmod,
	3,		MAXARGS,
#endif

#ifdef CMD_CHOWN
	"chown",	"uid filename ...",	do_chown,
	3,		MAXARGS,
#endif

#ifdef CMD_CMP
	"cmp",		"filename1 filename2",	do_cmp,
	3,		3,
#endif

#ifdef CMD_CP
	"cp",		"srcname ... destname",	do_cp,
	3,		MAXARGS,
#endif

#ifdef CMD_DD
	"dd",		"if=name of=name [bs=n] [count=n] [skip=n] [seek=n]", do_dd,
	3,		MAXARGS,
#endif

#ifdef CMD_ECHO
	"echo",	"[args] ...",		do_echo,
	1,		MAXARGS,
#endif

#ifdef CMD_ED
	"ed",		"[filename]",		do_ed,
	1,		2,
#endif

	"exec",		"filename [args]",	do_exec,
	2,		MAXARGS,

	"exit",		"",			do_exit,
	1,		1,

#ifdef CMD_GREP
	"grep",	"[-in] word filename ...",	do_grep,
	3,		MAXARGS,
#endif

#ifdef CMD_HELP
	"help",		"",			do_help,
	1,		MAXARGS,
#endif

#ifdef CMD_KILL
	"kill",	"[-sig] pid ...",	do_kill,
	2,		MAXARGS,
#endif

#ifdef CMD_LN
	"ln",		"[-s] srcname ... destname",	do_ln,
	3,		MAXARGS,
#endif

#ifdef CMD_LS
	"ls",		"[-lid] filename ...",	do_ls,
	1,		MAXARGS,
#endif

#ifdef CMD_MKDIR
	"mkdir",	"dirname ...",		do_mkdir,
	2,		MAXARGS,
#endif

#ifdef CMD_MKNOD
	"mknod",	"filename type major minor",	do_mknod,
	5,		5,
#endif

#ifdef CMD_MORE
	"more",	"filename ...",		do_more,
	2,		MAXARGS,
#endif

#ifdef CMD_MOUNT
	"mount",	"[-t type] devname dirname",	do_mount,
	3,		MAXARGS,
#endif

#ifdef CMD_MV
	"mv",		"srcname ... destname",	do_mv,
	3,		MAXARGS,
#endif

#ifdef CMD_PRINTENV
	"printenv",	"[name]",		do_printenv,
	1,		2,
#endif

#ifdef CMD_PROMPT
	"prompt",	"string",		do_prompt,
	2,		MAXARGS,
#endif

#ifdef CMD_PWD
	"pwd",		"",			do_pwd,
	1,		1,
#endif

	"quit",		"",			do_exit,
	1,		1,

#ifdef CMD_RM
	"rm",		"filename ...",		do_rm,
	2,		MAXARGS,
#endif

#ifdef CMD_RMDIR
	"rmdir",	"dirname ...",		do_rmdir,
	2,		MAXARGS,
#endif

#ifdef CMD_SETENV
	"setenv",	"name value",		do_setenv,
	2,		3,
#endif

#ifdef CMD_SOURCE
	"source",	"filename",		do_source,
	2,		2,
#endif

#ifdef CMD_SYNC
	"sync",	"",			do_sync,
	1,		1,
#endif

#ifdef CMD_TAR
	"tar",		"[xtv]f devname filename ...",	do_tar,
	2,		MAXARGS,
#endif

#ifdef CMD_TOUCH
	"touch",	"filename ...",		do_touch,
	2,		MAXARGS,
#endif

#ifdef CMD_UMASK
	"umask",	"[mask]",		do_umask,
	1,		2,
#endif

#ifdef CMD_MOUNT
	"umount",	"filename",		do_umount,
	2,		2,
#endif

#ifdef CMD_ALIAS
	"unalias",	"name",			do_unalias,
	2,		2,
#endif
	NULL,		0,			0,
	0,		0
};


#ifdef CMD_ALIAS
typedef struct {
	char	*name;
	char	*value;
} ALIAS;

static	ALIAS	*aliastable;
static	int	aliascount;
static	ALIAS	*findalias();
#endif

static	BOOL	intcrlf = TRUE;
static	char	*prompt;
static BOOL		isbinshell;

#ifdef CMD_SOURCE
static	FILE	*sourcefiles[MAXSOURCE];
static	int	sourcecount;
#endif

static	void	catchint();
static	void	catchtstp();
static	void	catchquit();
static	void	readfile();
static	void	command();
static	void	runcmd();
static	void	showprompt();
static	BOOL	trybuiltin();

BOOL	intflag;


int main(int argc, char **argv)
{
	char	*cp;
	char	buf[PATHLEN];

	signal(SIGINT, catchint);
	signal(SIGQUIT, catchquit);
	signal(SIGTSTP, SIG_IGN);

	/* check if we are /bin/sh*/
	if ((cp = strrchr(argv[0], '/')) != 0)
		cp++;
	else cp = argv[0];
	isbinshell = !strcmp(cp, "sh");

#ifdef CMD_PROMPT
	if ((prompt = malloc(3)) == 0)
#endif
		prompt = "  ";
	strcpy(prompt, getuid()? "$ ": "# ");

	if (getenv("PATH") == NULL)
		putenv("PATH=/bin:/usr/bin:/sbin");

#ifdef CMD_SOURCE
#ifdef LATER
	cp = getenv("HOME");
	if (cp) {
		strcpy(buf, cp);
		strcat(buf, "/");
		strcat(buf, ".sashrc");

		if ((access(buf, 0) == 0) || (errno != ENOENT))
			readfile(buf);
	}
#endif

	if (argc > 1) {
		readfile(argv[1]);
	} else {
		readfile(NULL);
	}
#else
	readfile(NULL);
#endif

	exit(0);
}


/*
 * Read commands from the specified file.
 * A null name pointer indicates to read from stdin.
 */
static void
readfile(name)
	char	*name;
{
	FILE	*fp;
	int	cc;
	char	buf[CMDLEN];
#ifdef CMD_SOURCE
	BOOL	ttyflag;

	if (sourcecount >= MAXSOURCE) {
		fprintf(stderr, "Too many source files\n");
		return;
	}

	fp = stdin;
	if (name) {
		fp = fopen(name, "r");
		if (fp == NULL) {
			perror(name);
			return;
		}
	}
	sourcefiles[sourcecount++] = fp;

	ttyflag = isatty(fileno(fp));
#else
	fp = stdin;
#endif

	while (TRUE) {
		fflush(stdout);
		showprompt();

#ifdef CMD_SOURCE
		if (intflag && !ttyflag && (fp != stdin)) {
			fclose(fp);
			sourcecount--;
			return;
		}
#endif

		if (fgets(buf, CMDLEN - 1, fp) == NULL) {
			if (ferror(fp) && (errno == EINTR)) {
				clearerr(fp);
				continue;
			}
			break;
		}

		cc = strlen(buf);
		if (buf[cc - 1] == '\n')
			cc--;

		while ((cc > 0) && isblank(buf[cc - 1]))
			cc--;
		buf[cc] = '\0';

		command(buf);
	}

	if (ferror(fp)) {
		perror("Reading command line");
#ifdef CMD_SOURCE
		if (fp == stdin)
			exit(1);
#else
		exit(1);
#endif
	}

	clearerr(fp);
#ifdef CMD_SOURCE
	if (fp != stdin)
		fclose(fp);

	sourcecount--;
#endif
}

/* check if command line requires a real shell to run it*/
static BOOL
needfullshell(char *cmd)
{
	char 	*cp;

	for (cp = cmd; *cp; cp++) {
		if ((*cp >= 'a') && (*cp <= 'z'))
			continue;
		if ((*cp >= 'A') && (*cp <= 'Z'))
			continue;
		if (isdecimal(*cp))
			continue;
		if (isblank(*cp))
			continue;

		if ((*cp == '.') || (*cp == '/') || (*cp == '-') ||
			(*cp == '+') || (*cp == '=') || (*cp == '_') ||
			(*cp == ':') || (*cp == ',') || (*cp == '#'))
				continue;
#ifdef WILDCARDS
		if ((*cp == '*') || (*cp == '?') || (*cp == '[') || (*cp == ']'))
			continue;
#endif

		return TRUE;
	}
	return FALSE;
}


/*
 * Parse and execute one null-terminated command line string.
 * This breaks the command line up into words, checks to see if the
 * command is an alias, and expands wildcards.
 */
static void
command(char *cmd)
{
	char	**argv;
	int	argc;
#ifdef CMD_ALIAS
	ALIAS	*alias;
	char	buf[CMDLEN];
#endif

	intflag = FALSE;
#ifdef WILDCARDS
	freechunks();
#endif

	while (isblank(*cmd))
		cmd++;

	if ((*cmd == '\0') || (*cmd == '#') || !makeargs(cmd, &argc, &argv))
		return;

#ifdef CMD_ALIAS
	/*
	 * Search for the command in the alias table.
	 * If it is found, then replace the command name with
	 * the alias, and append any other arguments to it.
	 */
	alias = findalias(argv[0]);
	if (alias) {
		cmd = buf;
		strcpy(cmd, alias->value);

		while (--argc > 0) {
			strcat(cmd, " ");
			strcat(cmd, *++argv);
		}

		if (!makeargs(cmd, &argc, &argv))
			return;
	}
#endif

	/*
	 * Now look for the command in the builtin table, and execute
	 * the command if found.
	 */
	if (!needfullshell(cmd) && trybuiltin(argc, argv))
		return;

	/*
	 * Not found, run the program along the PATH list.
	 */
	runcmd(cmd, argc, argv);
}

/*
 * Try to execute a built-in command.
 * Returns TRUE if the command is a built in, whether or not the
 * command succeeds.  Returns FALSE if this is not a built-in command.
 */
static BOOL
trybuiltin(int argc, char **argv)
{
	CMDTAB	*cmdptr;
#ifdef WILDCARDS
	int	oac;
	int	matches;
	int	newargc;
	int	i;
	char	*newargv[MAXARGS];
	char	*nametable[MAXARGS];
#endif

	cmdptr = cmdtab - 1;
	do {
		cmdptr++;
		if (cmdptr->name == NULL)
			return FALSE;

	} while (strcmp(argv[0], cmdptr->name));

	/*
	 * Give a usage string if the number of arguments is too large
	 * or too small.
	 */
	if ((argc < cmdptr->minargs) || (argc > cmdptr->maxargs)) {
		fprintf(stderr, "usage: %s %s\n", cmdptr->name, cmdptr->usage);
		return TRUE;
	}

	/*
	 * Check here for several special commands which do not
	 * have wildcarding done for them.
	 */
	if (
#ifdef CMD_ALIAS
	(cmdptr->func == do_alias) ||
#endif
#ifdef CMD_PROMPT
	(cmdptr->func == do_prompt) ||
#endif
	   0) {
		(*cmdptr->func)(argc, argv);
		return TRUE;
	}

#ifdef WILDCARDS
	/*
	 * Now for each command argument, see if it is a wildcard, and if
	 * so, replace the argument with the list of matching filenames.
	 */
	newargv[0] = argv[0];
	newargc = 1;
	oac = 0;

	while (++oac < argc) {
		matches = expandwildcards(argv[oac], MAXARGS, nametable);
		if (matches < 0)
			return TRUE;

		if ((newargc + matches) >= MAXARGS) {
			fprintf(stderr, "Too many arguments\n");
			return TRUE;
		}

		if (matches == 0)
			newargv[newargc++] = argv[oac];

		for (i = 0; i < matches; i++)
			newargv[newargc++] = nametable[i];
	}

	(*cmdptr->func)(newargc, newargv);
#else
	(*cmdptr->func)(argc, argv);

#endif
	return TRUE;
}


/*
 * Execute the specified command.
 */
static void
runcmd(char *cmd, int argc, char **argv)
{
	int		pid;
	int		status;

	if (needfullshell(cmd)) {
		if (isbinshell)
			printf("%s: no such file or directory\n", argv[0]);
		else {
			system(cmd);
			wait(&status);
		}
		return;
	}

	/*
	 * No magic characters in the command, so do the fork and
	 * exec ourself.  If this fails with ENOEXEC, then run the
	 * shell anyway since it might be a shell script.
	 */
	pid = vfork();

	if (pid < 0) {
		perror("fork failed");
		return;
	}

	if (pid) {
		status = 0;
		intcrlf = FALSE;

		while (((pid = wait(&status)) < 0) && (errno == EINTR))
			;

		intcrlf = TRUE;
		if ((status & 0xff) == 0)
			return;

		fprintf(stderr, "pid %d: %s (signal %d)\n", pid,
			(status & 0x80) ? "core dumped" : "killed",
			status & 0x7f);

		return;
	}

	/*
	 * We are the child, so run the program.
	 * First close any extra file descriptors we have opened.
	 */
#ifdef CMD_SOURCE
	while (--sourcecount >= 0) {
		if (sourcefiles[sourcecount] != stdin)
			fclose(sourcefiles[sourcecount]);
	}
#endif

	execvp(argv[0], argv);

	if (errno == ENOEXEC) {
		if (isbinshell)
			printf("%s: no such file or directory\n", argv[0]);
		else {
			system(cmd);
			wait(&status);
		}
		exit(0);
	}

	perror(argv[0]);
	exit(1);
}

#ifdef CMD_HELP
void
do_help(argc, argv)
	char	**argv;
{
	CMDTAB	*cmdptr;

	for (cmdptr = cmdtab; cmdptr->name; cmdptr++)
		printf("%-10s %s\n", cmdptr->name, cmdptr->usage);
}
#endif /* CMD_HELP */

#ifdef CMD_ALIAS
void
do_alias(argc, argv)
	char	**argv;
{
	char	*name;
	char	*value;
	ALIAS	*alias;
	int	count;
	char	buf[CMDLEN];

	if (argc < 2) {
		count = aliascount;
		for (alias = aliastable; count-- > 0; alias++)
			printf("%s\t%s\n", alias->name, alias->value);
		return;
	}

	name = argv[1];
	if (argc == 2) {
		alias = findalias(name);
		if (alias)
			printf("%s\n", alias->value);
		else
			fprintf(stderr, "Alias \"%s\" is not defined\n", name);
		return;
	}

	if (strcmp(name, "alias") == 0) {
		fprintf(stderr, "Cannot alias \"alias\"\n");
		return;
	}

	if (!makestring(argc - 2, argv + 2, buf, CMDLEN))
		return;

	value = malloc(strlen(buf) + 1);

	if (value == NULL) {
		fprintf(stderr, "No memory for alias value\n");
		return;
	}

	strcpy(value, buf);

	alias = findalias(name);
	if (alias) {
		free(alias->value);
		alias->value = value;
		return;
	}

	if ((aliascount % ALIASALLOC) == 0) {
		count = aliascount + ALIASALLOC;

		if (aliastable)
			alias = (ALIAS *) realloc(aliastable,
				sizeof(ALIAS *) * count);
		else
			alias = (ALIAS *) malloc(sizeof(ALIAS *) * count);

		if (alias == NULL) {
			free(value);
			fprintf(stderr, "No memory for alias table\n");
			return;
		}

		aliastable = alias;
	}

	alias = &aliastable[aliascount];

	alias->name = malloc(strlen(name) + 1);

	if (alias->name == NULL) {
		free(value);
		fprintf(stderr, "No memory for alias name\n");
		return;
	}

	strcpy(alias->name, name);
	alias->value = value;
	aliascount++;
}

/*
 * Look up an alias name, and return a pointer to it.
 * Returns NULL if the name does not exist.
 */
static ALIAS *
findalias(name)
	char	*name;
{
	ALIAS	*alias;
	int	count;

	count = aliascount;
	for (alias = aliastable; count-- > 0; alias++) {
		if (strcmp(name, alias->name) == 0)
			return alias;
	}

	return NULL;
}
#endif /* CMD_ALIAS */


#ifdef CMD_SOURCE
void
do_source(argc, argv)
	char	**argv;
{
	readfile(argv[1]);
}
#endif


void
do_exec(argc, argv)
	char	**argv;
{
	char	*name;

	name = argv[1];

	if (access(name, 4)) {
		perror(name);
		return;
	}

#ifdef CMD_SOURCE
	while (--sourcecount >= 0) {
		if (sourcefiles[sourcecount] != stdin)
			fclose(sourcefiles[sourcecount]);
	}
#endif

	argv[0] = name;
	argv[1] = NULL;

	execv(name, argv);

	perror(name);
	exit(1);
}

#ifdef CMD_PROMPT
void
do_prompt(argc, argv)
	char	**argv;
{
	char	*cp;
	char	buf[CMDLEN];

	if (!makestring(argc - 1, argv + 1, buf, CMDLEN))
		return;

	cp = malloc(strlen(buf) + 2);
	if (cp == NULL) {
		fprintf(stderr, "No memory for prompt\n");
		return;
	}

	strcpy(cp, buf);
	strcat(cp, " ");

	if (prompt)
		free(prompt);
	prompt = cp;
}
#endif

#ifdef CMD_ALIAS
void
do_unalias(argc, argv)
	char	**argv;
{
	ALIAS	*alias;

	while (--argc > 0) {
		alias = findalias(*++argv);
		if (alias == NULL)
			continue;

		free(alias->name);
		free(alias->value);
		aliascount--;
		alias->name = aliastable[aliascount].name;
		alias->value = aliastable[aliascount].value;
	}
}
#endif /* CMD_ALIAS */

/*
 * Display the prompt string.
 */
static void
showprompt()
{
	write(STDOUT, prompt, strlen(prompt));
}


static void
catchtstp()
{
	signal(SIGTSTP, catchtstp);

	intflag = TRUE;

	if (intcrlf)
		write(STDOUT, "\n", 1);
}

static void
catchint()
{
	signal(SIGINT, catchint);

	intflag = TRUE;

	if (intcrlf)
		write(STDOUT, "\n", 1);
}


static void
catchquit()
{
	signal(SIGQUIT, catchquit);

	intflag = TRUE;

	if (intcrlf)
		write(STDOUT, "\n", 1);
}

/* END CODE */
