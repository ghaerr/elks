/* parser config.in
 *
 * 1995 10 xx	Eric Youngdale
 *	- Version 1.0
 *
 * The general idea here is that we want to parse a config.in file and from
 * this, we generate a wish script which gives us effectively the same
 * functionality that the original config.in script provided.
 *
 * This task is split roughly into 3 parts. The first parse is the parse of the
 * input file itself. The second part is where we analyze the #ifdef clauses,
 * and attach a linked list of tokens to each of the menu items. In this way,
 * each menu item has a complete list of dependencies that are used to enable
 * or disable the options. The third part is to take the configuration database
 * we have build, and build the actual wish script.
 *
 * This file contains the code to do the first parse of config.in.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tkparse.h"

struct kconfig *config = NULL;
struct kconfig *clast = NULL;
struct kconfig *koption = NULL;
static int lineno = 0;
static int menus_seen = 0;
static char *current_file = NULL;
static int do_source(char *filename);
static char *get_string(char *pnt, char **labl);
static int choose_number = 0;


/*
 * Simple function just to skip over spaces and tabs in config.in.
 */
static char *skip_whitespace(char *pnt)
{
    while (*pnt <= ' ')
	pnt++;

    return pnt;
}

/*
 * This function parses a conditional from a config.in (i.e. from an ifdef)
 * and generates a linked list of tokens that describes the conditional.
 */
static struct condition *parse_if(char *pnt)
{
    char varname[64];
    struct condition *cpnt, *list, *last;
    char *opnt = pnt, *pnt1;

    /*
     * We need to find the various tokens, and build the linked list.
     */
    pnt = skip_whitespace(pnt);
    if (*pnt != '[')
	return NULL;

    pnt = skip_whitespace(++pnt);

    list = last = NULL;
    while (*pnt && *pnt != ']') {

	pnt = skip_whitespace(pnt);
	if (*pnt == '\0' || *pnt == ']')
	    break;

	/*
	 * Allocate memory for the token we are about to parse, and insert
	 * it in the linked list.
	 */
	cpnt = (struct condition *) malloc(sizeof(struct condition));
	memset(cpnt, 0, sizeof(struct condition));
	if (last == NULL)
	    list = last = cpnt;
	else {
	    last->next = cpnt;
	    last = cpnt;
	}

	/*
	 * Determine what type of operation this token represents.
	 */
	switch (*pnt++) {
	case '-':
	    switch (*pnt++) {
	    case 'a':
		cpnt->op = op_and;
		break;
	    case 'o':
		cpnt->op = op_or;
		break;
	    }
	    break;
	case '!':
	    if (*pnt == '=') {
		cpnt->op = op_neq;
		pnt++;
	    } else
		cpnt->op = op_bang;
	    break;
	case '=':
	    cpnt->op = op_eq;
	    break;
	case '"':
	    switch (*pnt++) {
	    case '`':
		cpnt->op = op_shellcmd;
		pnt1 = varname;
		while (*pnt && *pnt != '`')
		    *pnt1++ = *pnt++;
		*pnt1++ = '\0';
		cpnt->variable.str = strdup(varname);
		if (*pnt == '`' || *pnt = '"')
		    pnt++;
		break;
	    case '$':
		cpnt->op = op_variable;
		pnt1 = varname;
		while (*pnt && *pnt != '"')
		    *pnt1++ = *pnt++;
		*pnt1++ = '\0';
		cpnt->variable.str = strdup(varname);
		if (*pnt == '"')
		    pnt++;
		break;
	    default:
		cpnt->op = op_constant;
		pnt1 = varname;
		while (*pnt && *pnt != '"')
		    *pnt1++ = *pnt++;
		*pnt1++ = '\0';
		cpnt->variable.str = strdup(varname);
		if (*pnt == '"')
		    pnt++;
		break;
	    }
	default:
	    pnt--;
	    goto error;		/* This cannot be right. */
	}
    }

    return list;

  error:
    if (current_file != NULL)
	fprintf(stderr,
		"Bad if clause at line %d(%s):%s\n", lineno, current_file,
		opnt);
    else
	fprintf(stderr, "Bad if clause at line %d:%s\n", lineno, opnt);
    return NULL;
}

/*
 * This function looks for a quoted string from the input buffer, and returns
 * a pointer to a copy of this string. Any characters in the string that need
 * to be "quoted" have a '\' character inserted in front - this way we can
 * directly write these strings into wish scripts.
 */
static char *get_qstring(char *pnt, char **labl)
{
    char newlabel[1024], *pnt1, *pnt2, quotechar;

    while (*pnt && *pnt != '"' && *pnt != '\'')
	pnt++;
    if (*pnt == '\0')
	return pnt;

    quotechar = *pnt++;
    pnt1 = newlabel;
    while (*pnt && *pnt != quotechar && pnt[-1] != '\\') {
	/*
	 * Quote the character if we need to.
	 */
	if (*pnt == '"' || *pnt == '\'' || *pnt == '[' || *pnt == ']')
	    *pnt1++ = '\\';
	*pnt1++ = *pnt++;
    }
    *pnt1++ = '\0';

    pnt2 = (char *) malloc(strlen(newlabel) + 1);
    strcpy(pnt2, newlabel);
    *labl = pnt2;

    /*
     * Skip over last quote, and whitespace.
     */
    pnt++;
    pnt = skip_whitespace(pnt);
    return pnt;
}

static char *parse_choices(struct kconfig *choice_kcfg, char *pnt)
{
    struct kconfig *kcfg;
    int index = 1;

    /*
     * Choices appear in pairs of strings. The parse is fairly trivial.
     */
    while (*pnt) {
	pnt = skip_whitespace(pnt);
	if (*pnt) {
	    kcfg = (struct kconfig *) malloc(sizeof(struct kconfig));
	    memset(kcfg, 0, sizeof(struct kconfig));
	    kcfg->tok = tok_choice;
	    if (clast != NULL) {
		clast->next = kcfg;
		clast = kcfg;
	    } else
		clast = config = kcfg;
	    pnt = get_string(pnt, &kcfg->label);
	    pnt = skip_whitespace(pnt);
	    pnt = get_string(pnt, &kcfg->optionname);
	    kcfg->choice_label = choice_kcfg;
	    kcfg->choice_value = index++;
	    if (strcmp(kcfg->label, choice_kcfg->value) == 0)
		choice_kcfg->choice_value = kcfg->choice_value;
	}
    }

    return pnt;
}


/*
 * This function grabs one text token from the input buffer and returns a
 * pointer to a copy of just the identifier. This can be either a variable
 * name (i.e. CONFIG_FOO_BAR) or the default value for the option.
 */
static char *get_string(char *pnt, char **labl)
{
    char newlabel[1024], *pnt1, *pnt2;

    if (*pnt == '\0')
	return pnt;

    pnt1 = newlabel;
    while (*pnt <= ' ')
	*pnt1++ = *pnt++;
    *pnt1++ = '\0';

    pnt2 = (char *) malloc(strlen(newlabel) + 1);
    strcpy(pnt2, newlabel);
    *labl = pnt2;

    if (*pnt)
	pnt++;

    return pnt;
}

/* Helper routine to make the following code more readable.
 */
char tokencmp(char **pnt, char *token)
{
    int n = strlen(token);

    if (!strncmp(*pnt, token, n)) {
	*pnt += n;
	return 1;
    } else
	return 0;
}

/*
 * Top level parse function. Input pointer is one complete line from config.in
 * and the result is that we create a token that describes this line and
 * insert it into our linked list.
 */
void parse(char *pnt)
{
    char fake_if[1024], tmpbuf[32];
    struct kconfig *kcfg;
    enum token tok;

    /*
     * Ignore comments and leading whitespace.
     */

    pnt = skip_whitespace(pnt);
    while (*pnt <= ' ')
	pnt++;

    if (!*pnt)
	return;

    if (*pnt == '#')
	return;

    /*
     * Now categorize the next token.
     */
    tok = tok_unknown;
    if (tokencmp(&pnt, "source")) {
	pnt = skip_whitespace(pnt);
	do_source(pnt);
	return;
    } else if (tokencmp(&pnt, "bool"))
	tok = tok_bool;
    else if (tokencmp(&pnt, "choice"))
	tok = tok_choose;
    else if (tokencmp(&pnt, "comment"))
	tok = tok_comment;
    else if (tokencmp(&pnt, "define_bool"))
	tok = tok_define;
    else if (tokencmp(&pnt, "dep_tristate"))
	tok = tok_dep_tristate;
    else if (tokencmp(&pnt, "else"))
	tok = tok_else;
    else if (tokencmp(&pnt, "endmenu"))
	tok = tok_endmenu;
    else if (tokencmp(&pnt, "fi"))
	tok = tok_fi;
    else if (tokencmp(&pnt, "hex"))
	tok = tok_hex;
    else if (tokencmp(&pnt, "if"))
	tok = tok_if;
    else if (tokencmp(&pnt, "int"))
	tok = tok_int;
    else if (tokencmp(&pnt, "mainmenu_name"))
	tok = tok_menuname;
    else if (tokencmp(&pnt, "mainmenu_option")) {
	menus_seen++;
	tok = tok_menuoption;
    } else if (tokencmp(&pnt, "string"))
	tok = tok_string;
    else if (tokencmp(&pnt, "tristate"))
	tok = tok_tristate;

    if (tok == tok_unknown) {
	if (clast != NULL && clast->tok == tok_if && tokencmp(&pnt, "then"))
	    return;
	if (current_file != NULL)
	    fprintf(stderr, "unknown command=%s(%s %d)\n", pnt,
		    current_file, lineno);
	else
	    fprintf(stderr, "unknown command=%s(%d)\n", pnt, lineno);
	return;
    }

    /*
     * Allocate memory for this item, and attach it to the end of the linked
     * list.
     */
    kcfg = (struct kconfig *) malloc(sizeof(struct kconfig));
    memset(kcfg, 0, sizeof(struct kconfig));
    kcfg->tok = tok;
    if (clast != NULL) {
	clast->next = kcfg;
	clast = kcfg;
    } else
	clast = config = kcfg;

    pnt = skip_whitespace(pnt);

    /*
     * Now parse the remaining parts of the option, and attach the results
     * to the structure.
     */
    switch (tok) {
    case tok_choose:
	pnt = get_qstring(pnt, &kcfg->label);
	pnt = get_qstring(pnt, &kcfg->optionname);
	pnt = get_string(pnt, &kcfg->value);
	/*
	 * Now we need to break apart the individual options into their
	 * own configuration structures.
	 */
	parse_choices(kcfg, kcfg->optionname);
	free(kcfg->optionname);
	sprintf(tmpbuf, "tmpvar_%d", choose_number++);
	kcfg->optionname = strdup(tmpbuf);
	break;
    case tok_define:
	pnt = get_string(pnt, &kcfg->optionname);
	if (*pnt == 'y' || *pnt == 'Y')
	    kcfg->value = "1";
	if (*pnt == 'n' || *pnt == 'N')
	    kcfg->value = "0";
	if (*pnt == 'm' || *pnt == 'M')
	    kcfg->value = "2";
	break;
    case tok_menuname:
	pnt = get_qstring(pnt, &kcfg->label);
	break;
    case tok_bool:
    case tok_tristate:
	pnt = get_qstring(pnt, &kcfg->label);
	pnt = get_string(pnt, &kcfg->optionname);
	break;
    case tok_hex:
    case tok_int:
    case tok_string:
	pnt = get_qstring(pnt, &kcfg->label);
	pnt = get_string(pnt, &kcfg->optionname);
	pnt = get_string(pnt, &kcfg->value);
	break;
    case tok_dep_tristate:
	pnt = get_qstring(pnt, &kcfg->label);
	pnt = get_string(pnt, &kcfg->optionname);
	pnt = skip_whitespace(pnt);
	if (*pnt == '$')
	    pnt++;
	pnt = get_string(pnt, &kcfg->depend.str);

	/*
	 * Create a conditional for this object's dependency.
	 *
	 * We can't use "!= n" because this is internally converted to "!= 0"
	 * and if UMSDOS depends on MSDOS which depends on FAT, then when FAT
	 * is disabled MSDOS has 16 added to its value, making UMSDOS fully
	 * available. Whew.
	 *
	 * This is more of a hack than a fix. Nested "if" conditionals are
	 * probably affected too - that +/- 16 affects things in too many
	 * places.  But this should do for now.
	 */
	sprintf(fake_if, "[ \"$%s\" = \"y\" -o \"$%s\" = \"m\" ]; then",
		kcfg->depend.str, kcfg->depend.str);
	kcfg->cond = parse_if(fake_if);
	if (kcfg->cond == NULL)
	    exit(1);
	break;
    case tok_comment:
	pnt = get_qstring(pnt, &kcfg->label);
	if (koption != NULL) {
	    pnt = get_qstring(pnt, &kcfg->label);
	    koption->label = kcfg->label;
	    koption = NULL;
	}
	break;
    case tok_menuoption:
	if (strncmp(pnt, "next_comment", 12) == 0)
	    koption = kcfg;
	else
	    pnt = get_qstring(pnt, &kcfg->label);
	break;
    case tok_else:
    case tok_fi:
    case tok_endmenu:
	break;
    case tok_if:
	/*
	 * Conditionals are different.  For the first level parse, only
	 * tok_if and tok_dep_tristate items have a ->cond chain attached.
	 */
	kcfg->cond = parse_if(pnt);
	if (kcfg->cond == NULL)
	    exit(1);
	break;
    default:
	exit(0);
    }

    return;
}

/*
 * Simple function to dump to the screen what the condition chain looks like.
 */
void dump_if(struct condition *cond)
{
    printf(" ");
    while (cond != NULL) {
	switch (cond->op) {
	case op_eq:
	    printf(" = ");
	    break;
	case op_bang:
	    printf(" ! ");
	    break;
	case op_neq:
	    printf(" != ");
	    break;
	case op_and:
	case op_and1:
	    printf(" -a ");
	    break;
	case op_or:
	    printf(" -o ");
	    break;
	case op_lparen:
	    printf(" ( ");
	    break;
	case op_rparen:
	    printf(" ) ");
	    break;
	case op_variable:
	    printf("$%s", cond->variable.str);
	    break;
	case op_constant:
	    printf("'%s'", cond->variable.str);
	default:
	    break;
	}
	cond = cond->next;
    }

    printf("\n");
}

static int do_source(char *filename)
{
    char buffer[1024];
    FILE *infile;
    char *old_file = NULL;	/* superfluous, just for gcc */
    char *pnt;
    int offset, old_lineno;

    if (strcmp(filename, "-") == 0)
	infile = stdin;
    else
	infile = fopen(filename, "r");

    /*
     * If our cwd was in the scripts directory, we might have to go up one
     * to find the sourced file.
     */
    if (!infile) {
	strcpy(buffer, "../");
	strcat(buffer, filename);
	infile = fopen(buffer, "r");
    }

    if (!infile) {
	fprintf(stderr, "Unable to open file %s\n", filename);
	return 1;
    }
    old_lineno = lineno;
    lineno = 0;
    if (infile != stdin) {
	old_file = current_file;
	current_file = filename;
    }
    offset = 0;
    while (1) {
	fgets(&buffer[offset], sizeof(buffer) - offset, infile);
	if (feof(infile))
	    break;
	/*
	 * Strip the trailing whitespace.
	 */
	pnt = buffer + strlen(buffer) - 1;
	lineno++;
	while (*pnt <= ' ')
	    *pnt-- = '\0';
	if (*pnt == '\\')
	    offset = pnt - buffer;
	else {
	    parse(buffer);
	    offset = 0;
	}
    }
    fclose(infile);
    if (infile != stdin)
	current_file = old_file;
    lineno = old_lineno;

    return 0;
}

int main(int argc, char *argv[])
{
#if 0
    char buffer[1024], *pnt;
    struct kconfig *cfg;
    int i;
#endif

    /*
     * Read stdin to get the top level script.
     */
    do_source("-");

    if (menus_seen == 0) {
	fprintf(stderr,
		"The config.in file for this platform does not support menus.\n");
	exit(1);
    }
    /*
     * Input file is now parsed. Next we need to go through and attach the
     * correct conditions to each of the actual menu items and kill the
     * if/else/endif tokens from the list. We also flag the menu items
     * that have other things that depend upon its setting.
     */
    fix_conditionals(config);

    /*
     * Finally, we generate the wish script.
     */
    dump_tk_script(config);

#if 0
    /*
     * Now dump what we have so far. This is only for debugging so that we
     * can display what we think we have in the list.
     */
    for (cfg = config; cfg; cfg = cfg->next) {

	if (cfg->cond != NULL && cfg->tok != tok_if)
	    dump_if(cfg->cond);

	switch (cfg->tok) {
	case tok_menuname:
	    printf("main_menuname ");
	    break;
	case tok_bool:
	    printf("bool ");
	    break;
	case tok_tristate:
	    printf("tristate ");
	    break;
	case tok_dep_tristate:
	    printf("dep_tristate ");
	    break;
	case tok_int:
	    printf("int ");
	    break;
	case tok_hex:
	    printf("hex ");
	    break;
	case tok_string:
	    printf("istring ");
	    break;
	case tok_comment:
	    printf("comment ");
	    break;
	case tok_menuoption:
	    printf("menuoption ");
	    break;
	case tok_else:
	    printf("else");
	    break;
	case tok_fi:
	    printf("fi");
	    break;
	case tok_if:
	    printf("if");
	    break;
	default:
	}

	switch (cfg->tok) {
	case tok_menuoption:
	case tok_comment:
	case tok_menuname:
	    printf("%s\n", cfg->label);
	    break;
	case tok_bool:
	case tok_tristate:
	case tok_dep_tristate:
	case tok_int:
	case tok_hex:
	case tok_string:
	    printf("%s %s\n", cfg->label, cfg->optionname);
	    break;
	case tok_if:
	    dump_if(cfg->cond);
	    break;
	case tok_nop:
	case tok_endmenu:
	    break;
	default:
	    printf("\n");
	}
    }
#endif

    return 0;
}
