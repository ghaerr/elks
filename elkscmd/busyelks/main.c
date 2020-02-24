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
#if defined(CMD_cat)
	CMD("cat",	cat_main,	"[file]...", "Concatenate files and print on the standard output."),
#endif
#if defined(CMD_chgrp)
	CMD("chgrp",	chgrp_main,	"group_name file1 [file2] ...", "Change group ownership."),
#endif
#if defined(CMD_chmod)
	CMD("chmod",	chmod_main,	"mode file1 [file2] ...", "Change file mode bits."),
#endif
#if defined(CMD_chown)
	CMD("chown",	chown_main,	"new_owner file1 [file2] ...", "Change file owner."),
#endif
#if defined(CMD_cksum)
	CMD("cksum",	cksum_main,	"[file1] ...", "Checksum and count the bytes in a file."),
#endif
#if defined(CMD_cmp)
	CMD("cmp",	cmp_main,	"file1 file2", "Compare two files byte by byte."),
#endif
#if defined(CMD_cp)
	CMD("cp",	cp_main,	"source_file1 [source_file2]... {dest_file | dest_directory}", "Copy files."),
#endif
#if defined(CMD_cut)
	CMD("cut",	cut_main,	"[-f args [-i] [-d x]]|[-c args] [filename [...]]",
		"Remove sections from each line of files."),
#endif
#if defined(CMD_date)
	CMD("date",	date_main,	"[option] [[yy]yy-m-dTh:m:s]",
		"Read or modify current system date\n"
		"-s STR\tset time described by STR\n"
		"-i\tinteractively set time\n"
		"-c STR\tinteractively set time if `now' is before STR"),
#endif
#if defined(CMD_dd)
	CMD("dd",	dd_main,	"if=<inflie> of=<outfile> [bs=<N>] [count=<N>] [seek=<N>] [skip=<N>]",
		"Copy a file."),
#endif
#if defined(CMD_diff)
	CMD("diff",	diff_main, "[-c|-e|-C n][-br] file1 file2", "Compare files line by line."),
#endif
#if defined(CMD_dirname)
	CMD("dirname",	dirname_main,	"NAME", " Strip last component from file name."),
#endif
#if defined(CMD_du)
	CMD("du",	du_main,	" [-a] [-s] [-l levels] [startdir]", "Estimate file space usage."),
#endif
#if defined(CMD_echo)
	CMD("echo",	echo_main, NULL, "Display a line of text."),
#endif
#if defined(CMD_ed)
	CMD("ed",	ed_main, "[file]", "Line-oriented text editor"),
#endif
#if defined(CMD_false)
	CMD("false",	false_main,	NULL, "Do nothing, unsuccessfully."),
#endif
#if defined(CMD_fdisk)
	CMD("fdisk",	fdisk_main,	"[-l] device", "Manipulate disk partition table."),
#endif
#if defined(CMD_find)
	CMD("find",	find_main,	"path-list [predicate-list]", "Search for files in a directory hierarchy."),
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
				char const * s;

				fputs("\n\t\t", stderr);

				for(s = cmd[i].descr; *s; s++)
				{
					fputc(*s, stderr);
					if(*s == '\n')
						fputs("\t\t", stderr);
				}
			}
#	endif
#endif
			fputc('\n', stderr);
		}
	}

	return 1;
}
