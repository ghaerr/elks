/*
 *  linux/fs/binfmt_elks.c
 *
 *  Copyright (C) 1996  Martin von Löwis
 *  original #!-checking implemented by tytso.
 *  ELKS hack added by Chad Page...
 *  Cleaned up some more by Robert de Bath 
 */

#include <linux/module.h>
#ifndef LINUX_VERSION_CODE
#include <linux/version.h>
#endif
#include <linux/string.h>
#include <linux/stat.h>
#include <linux/malloc.h>
#include <linux/binfmts.h>

static int do_load_elksaout(struct linux_binprm *bprm,struct pt_regs *regs)
{
	char *cp, *interp, *i_name, *i_arg;
	int retval;

	/* What a horrible hack! :-) */
	if ((bprm->buf[0] != 1)   || (bprm->buf[1] != 3) || 
	    (bprm->buf[2] != 0x20 && bprm->buf[2] != 0x10) ||
	    (bprm->buf[3] != 4) || bprm->sh_bang)
		return -ENOEXEC;

	bprm->sh_bang++;
	iput(bprm->inode);
#if LINUX_VERSION_CODE >= 0x010300
	bprm->dont_iput=1;
#endif

	interp = "/lib/elksemu";
	i_name = interp;
	i_arg = 0;

	for (cp=i_name; *cp && (*cp != ' ') && (*cp != '\t'); cp++) {
 		if (*cp == '/')
			i_name = cp+1;
	}

	/*
	 * OK, we've parsed out the interpreter name and
	 * (optional) argument.
	 * Splice in (1) the interpreter's name for argv[0]
	 *           (2) (optional) argument to interpreter
	 *           (3) filename of shell script (replace argv[0])
	 *
	 * This is done in reverse order, because of how the
	 * user environment and arguments are stored.
	 */
	remove_arg_zero(bprm);
	bprm->p = copy_strings(1, &bprm->filename, bprm->page, bprm->p, 2);
	bprm->argc++;
	if (i_arg) {
		bprm->p = copy_strings(1, &i_arg, bprm->page, bprm->p, 2);
		bprm->argc++;
	}
	bprm->p = copy_strings(1, &i_name, bprm->page, bprm->p, 2);
	bprm->argc++;
	if (!bprm->p) 
		return -E2BIG;
	/*
	 * OK, now restart the process with the interpreter's inode.
	 * Note that we use open_namei() as the name is now in kernel
	 * space, and we don't need to copy it.
	 */
	retval = open_namei(interp, 0, 0, &bprm->inode, NULL);
	if (retval)
		return retval;
#if LINUX_VERSION_CODE >= 0x010300
	bprm->dont_iput=0;
#endif
	retval=prepare_binprm(bprm);
	if(retval<0)
		return retval;
	return search_binary_handler(bprm,regs);
}

static int load_elksaout(struct linux_binprm *bprm,struct pt_regs *regs)
{
	int retval;
	MOD_INC_USE_COUNT;
	retval = do_load_elksaout(bprm,regs);
	MOD_DEC_USE_COUNT;
	return retval;
}

struct linux_binfmt elksaout_format = {
#ifndef MODULE
	NULL, 0, load_elksaout, NULL, NULL
#else
	NULL, &mod_use_count_, load_elksaout, NULL, NULL
#endif
};

int init_elksaout_binfmt(void) {
	return register_binfmt(&elksaout_format);
}

#ifdef MODULE
#if LINUX_VERSION_CODE < 0x010300
char kernel_version[] = UTS_RELEASE;
#endif
int init_module(void)
{
	return init_elksaout_binfmt();
}

void cleanup_module( void) {
	unregister_binfmt(&elksaout_format);
}
#endif
