#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "lib.h"

#if defined(CMD_false)
int false_main(int argc, char * argv[])
{
	return 1;
}
#endif

#if defined(CMD_true)
int true_main(int argc, char * argv[])
{
	return 0;
}
#endif

struct cmd
{
	char const *	name;
	int (* fun)(int argc, char * argv[]);
#if defined(CMD_INFO_ARGS)
	char const *	args;
#	if defined(CMD_INFO_DESCR)
	char const *	descr;
#	endif
#endif
};

#if defined(CMD_INFO_ARGS)
#	if defined(CMD_INFO_DESCR)
#		define	CMD(name, fun, args, descr)	{ name, fun, args, descr }
#	else
#		define	CMD(name, fun, args, descr)	{ name, fun, args }
#	endif
#else
#	define	CMD(name, fun, args, descr)	{ name, fun }
#endif

#define	CMD_MAX	(sizeof(cmd) / sizeof(*cmd))

static struct cmd cmd[] =
{
#if defined(CMD_basename)
	CMD("basename",	basename_main,	"NAME [SUFFIX]", "Strip directory and suffix from filenames."),
#endif
#if defined(CMD_cal)
	CMD("cal",	cal_main,	"[month] year", "Displays a calendar."),
#endif
#if defined(CMD_dirname)
	CMD("dirname",	dirname_main,	"NAME", " Strip last component from file name."),
#endif
#if defined(CMD_false)
	CMD("false",	false_main,	NULL, "Do nothing, unsuccessfully."),
#endif
#if defined(CMD_true)
	CMD("true",	true_main, NULL, "Do nothing, successfully"),
#endif
};

static int
cmd_cmp(void const * key, void const * data)
{
	return strcmp((char const *)key, ((struct cmd *)data)->name);
}

int
main(int argc, char * argv[])
{
	char * progname = strrchr(argv[0], '/');

	if(!progname || (progname == argv[0] && !progname[1]))
		progname = argv[0];
	else
		progname++;

	/* If our name was called, assume argv[1] is the command */
	if(strcmp(progname, "busyelks") == 0) {
		progname = argv[1];
		argv++;
		argc--;
	}

	{
		struct cmd * c = bsearch(progname, cmd, CMD_MAX, sizeof(struct cmd), cmd_cmp);

		if(c != NULL)
			return c->fun(argc, argv);
	}

	fputs("BusyELKS", stderr);

	if(*progname)
	{
		fputs(": Unknown command: ", stderr);
		fputs(progname, stderr);
	}

	fputc('\n' , stderr);
	{
		unsigned i;

		for(i = 0; i < CMD_MAX; i++)
		{
			fputc('\t', stderr);
			fputs(cmd[i].name, stderr);
#if defined(CMD_INFO_ARGS)
			fputc(' ', stderr);
			if(cmd[i].args != NULL)
				fputs(cmd[i].args, stderr);
#	if defined(CMD_INFO_DESCR)
			if(cmd[i].descr != NULL)
			{
				fputs("\n\t\t", stderr);
				fputs(cmd[i].descr, stderr);
			}
#	endif
#endif
			fputc('\n', stderr);
		}
	}

	return 1;
}
