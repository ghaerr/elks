/*
   footobar.c: a collection of functions to convert internal representations of
   variables and functions to external representations, and vice versa
*/

#include "rc.h"

/* protect an exported name from brain-dead shells */

#ifdef PROTECT_ENV
static bool Fconv(Format *f, int ignore) {
	unsigned const char *s = va_arg(f->args, unsigned const char *);
	int c;

	while ((c = *s++) != '\0')
		if (dnw[c] || c == '*' || (c == '_' && *s == '_'))
			fmtprint(f, "__%02x", c);
		else
			fmtputc(f, c);
	return FALSE;
}
#endif

/* convert a redirection to a printable form */

static bool Dconv(Format *f, int ignore) {
	char *name = "?";
	int n = va_arg(f->args, int);
	switch (n) {
	case rCreate:		name = ">";	break;
	case rAppend:		name = ">>";	break;
	case rFrom:		name = "<";	break;
	case rHeredoc:		name = "<<";	break;
	case rHerestring:	name = "<<<";	break;
	}
	fmtcat(f, name);
	return FALSE;
}

/* defaultfd -- return the default fd for a given redirection operation */

extern int defaultfd(int op) {
	return (op == rCreate || op == rAppend) ? 1 : 0;
}

/* convert a function in Node * form into something rc can parse (and humans can read?) */

static bool Tconv(Format *f, int ignore) {
	Node *n = va_arg(f->args, Node *);
	if (n == NULL) {
		fmtprint(f, "()");
		return FALSE;
	}
	switch (n->type) {
	case nWord:	fmtprint(f, "%S", n->u[0].s);				break;
	case nQword:	fmtprint(f, "%#S", n->u[0].s);				break;
	case nBang:	fmtprint(f, "!%T", n->u[0].p);				break;
	case nCase:	fmtprint(f, "case %T", n->u[0].p);			break;
	case nNowait:	fmtprint(f, "%T&", n->u[0].p);				break;
	case nRmfn:	fmtprint(f, "fn %T", n->u[0].p);			break;
	case nSubshell:	fmtprint(f, "@ %T", n->u[0].p);				break;
	case nAndalso:	fmtprint(f, "%T&&%T", n->u[0].p, n->u[1].p);		break;
	case nAssign:	fmtprint(f, "%T=%T", n->u[0].p, n->u[1].p);		break;
	case nConcat:	fmtprint(f, "%T^%T", n->u[0].p, n->u[1].p);		break;
	case nElse:	fmtprint(f, "{%T}else %T", n->u[0].p, n->u[1].p);	break;
	case nNewfn:	fmtprint(f, "fn %T {%T}", n->u[0].p, n->u[1].p);	break;
	case nIf:	fmtprint(f, "if(%T)%T", n->u[0].p, n->u[1].p);		break;
	case nOrelse:	fmtprint(f, "%T||%T", n->u[0].p, n->u[1].p);		break;
	case nArgs:	fmtprint(f, "%T %T", n->u[0].p, n->u[1].p);		break;
	case nSwitch:	fmtprint(f, "switch(%T){%T}", n->u[0].p, n->u[1].p);	break;
	case nMatch:	fmtprint(f, "~ %T %T", n->u[0].p, n->u[1].p);		break;
	case nWhile:	fmtprint(f, "while(%T)%T", n->u[0].p, n->u[1].p);	break;
	case nLappend:	fmtprint(f, "(%T %T)", n->u[0].p, n->u[1].p);		break;
	case nForin:	fmtprint(f, "for(%T in %T)%T", n->u[0].p, n->u[1].p, n->u[2].p); break;
	case nVarsub:	fmtprint(f, "$%T(%T)", n->u[0].p, n->u[1].p);		break;
	case nCount: case nFlat: case nVar: {
		char *lp = "", *rp = "";
		Node *n0 = n->u[0].p;

		if (n0->type != nWord && n0->type != nQword)
			lp = "(", rp = ")";

		switch (n->type) {
		default:	panic("this can't happen");		break;
		case nCount:	fmtprint(f, "$#%s%T%s", lp, n0, rp);	break;
		case nFlat:	fmtprint(f, "$^%s%T%s", lp, n0, rp);	break;
		case nVar:	fmtprint(f, "$%s%T%s", lp, n0, rp);	break;
		}
		break;
	}
	case nDup:
		if (n->u[2].i != -1)
			fmtprint(f, "%D[%d=%d]", n->u[0].i, n->u[1].i, n->u[2].i);
		else
			fmtprint(f, "%D[%d=]", n->u[0].i, n->u[1].i);
		break;
	case nBackq: {
		Node *n0 = n->u[0].p, *n00;
		if (n0 != NULL && n0->type == nVar
		    && (n00 = n0->u[0].p) != NULL && n00->type == nWord && streq(n00->u[0].s, "ifs"))
			fmtprint(f, "`");
		else
			fmtprint(f, "``%T", n0);
		fmtprint(f, "{%T}", n->u[1].p);
		break;
	}
	case nCbody:
	case nBody: {
		Node *n0 = n->u[0].p;
		if (n0 != NULL)
			fmtprint(f, "%T", n->u[0].p);
		if (n->u[1].p != NULL) {
			if (n0 != NULL && n0->type != nNowait)
				fmtprint(f, ";");
			fmtprint(f, "%T", n->u[1].p);
		}
		break;
	}
	case nBrace:
		fmtprint(f, "{%T}", n->u[0].p);
		if (n->u[1].p != NULL)
			fmtprint(f, "%T", n->u[1].p);
		break;
	case nEpilog:
	case nPre:
		fmtprint(f, "%T", n->u[0].p);
		if (n->u[1].p != NULL)
			fmtprint(f, " %T", n->u[1].p);
		break;
	case nPipe: {
		int ofd = n->u[0].i, ifd = n->u[1].i;
		fmtprint(f, "%T|", n->u[2].p);
		if (ifd != 0)
			fmtprint(f, "[%d=%d]", ofd, ifd);
		else if (ofd != 1)
			fmtprint(f, "[%d]", ofd);
		fmtprint(f, "%T", n->u[3].p);
		break;
	}
	case nRedir: {
		int op = n->u[0].i;
		fmtprint(f, "%D", op);
		if (n->u[1].i != defaultfd(op))
			fmtprint(f, "[%d]", n->u[1].i);
		fmtprint(f, "%T", n->u[2].p);
		break;
	}
	case nNmpipe: {
		int op = n->u[0].i;
		fmtprint(f, "%D", op);
		if (n->u[1].i != defaultfd(op))
			fmtprint(f, "[%d]", n->u[1].i);
		fmtprint(f, "{%T}", n->u[2].p);
		break;
	}
 	}
	return FALSE;
}

/* convert a List to an array, for execve() */

extern char **list2array(List *s, bool print) {
	char **argv, **av;

	if (print)
		fprint(2, "%L\n", s, " ");
	/*
	   Allocate 3 extra spots (2 for the fake execve & 1 for defaulting to
	   sh) and hide these from exec().
	*/
	argv = av = (char **) nalloc((listnel(s) + 4) * sizeof *av) + 3;
	while (s != NULL) {
		*av++ = s->w;
		s = s->n;
	}
	*av = NULL;
	return argv;
}

/* figure out the name of a variable given an environment string. */

extern char *get_name(char *s) {
	char *eq = strchr(s, '=');
	char *r, *result;
	int c;
	
	if (eq == NULL)
		return NULL;
	r = result = nalloc(eq - s + 1);
	while (1)
		switch (c = *s++) {
		case '=':
			*r++ = '\0';
			return result;
#ifdef PROTECT_ENV
		case '_':
			if (*s == '_') {
				static const char hexchar[] = "0123456789abcdef";
				char *h1 = strchr(hexchar, s[1]);
				char *h2 = strchr(hexchar, s[2]);
				if (h1 != NULL && h2 != NULL) {
					*r++ = ((h1 - hexchar) << 4) | (h2 - hexchar);
					s += 3;
					break;
				}
			}
			/* FALLTHROUGH */
#endif
		default:
			*r++ = c;
			break;
		}
}

/*
   Hacky routine to split a ^A-separated environment variable. Scans
   backwards. Be VERY CAREFUL with the loop boundary conditions. e.g.,
   notice what happens when there is no ^A in the extdef. (It does
   the right thing, of course :-)
*/

#define skipleft(p) do --p; while (*p != '\0' && *p != '\001');

extern List *parse_var(char *name, char *extdef) {
	char *endp, *realend, *sepp;
	List *tailp, *new;

	extdef = strchr(extdef, '=');
	*extdef++ = '\0'; /* null "initiate" the string for rtol scanning */

	sepp = realend = strchr(extdef, '\0');
	tailp = NULL;

	while (1) {
		endp = sepp;	/* set endp to end of current mebmer */
		skipleft(sepp);	/* advance sepp to the previous \1,  */
		*endp = '\0';   /* and null terminate the member.    */

		new = enew(List);
		new->w = ecpy(sepp+1);
		new->m = NULL;
		new->n = tailp;
		tailp = new;

		if (sepp < extdef)	/* break when sepp hits the null "initiator" */
			break;
		if (endp != realend)	/* else restore the \1 in between members */
			*endp = '\001';
	}
	if (endp != realend)
		*endp = '\001';
	*--extdef = '='; /* restore extdef '=' */
	return tailp;
}

/* get an environment entry for a function and have rc parse it. */

#define PREFIX "fn x"
#define PRELEN conststrlen(PREFIX)
extern Node *parse_fn(char *name, char *extdef) {
	Node *def;
	char *s, old[PRELEN];
	if ((s = strchr(extdef, '=')) == NULL)
		return NULL;
	memcpy(old, s -= (PRELEN-1), PRELEN);
	memcpy(s, PREFIX, PRELEN);
	def = parseline(s);
	memcpy(s, old, PRELEN);
	return (def == NULL || def->type != nNewfn) ? NULL : def->u[1].p;
}

static bool Aconv(Format *f, int c) {
	char **a = va_arg(f->args, char **);
	if (*a != NULL) {
		fmtcat(f, *a);
		while (*++a != NULL)
			fmtprint(f, " %s", *a);
	}
	return FALSE;
}

static bool Lconv(Format *f, int c) {
	List *l = va_arg(f->args, List *);
	char *sep = va_arg(f->args, char *);
	char *fmt = (f->flags & FMT_leftside) ? "%s%s" : "%-S%s";
	if (l == NULL && (f->flags & FMT_leftside) == 0)
		fmtprint(f, "()");
	else {
		List *s;
		for (s = l; s != NULL; s = s->n)
			fmtprint(f, fmt, s->w, s->n == NULL ? "" : sep);
	}
	return FALSE;
}

#define	ISMETA(c)	(c == '*' || c == '?' || c == '[')

static bool Sconv(Format *f, int ignore) {
	int c;
	unsigned char *s = va_arg(f->args, unsigned char *), *t = s;
	bool quoted    = (f->flags & FMT_altform)  != 0;	/* '#' */
	bool metaquote = (f->flags & FMT_leftside) != 0;	/* '-' */
	if (*s == '\0') {
		fmtprint(f, "''");
		return FALSE;
	}
	if (!quoted) {
		while ((c = *t++) != '\0')
			if (nw[c] == 1 || (metaquote && ISMETA(c)))
				goto quoteit;
		fmtprint(f, "%s", s);
		return FALSE;
	}
quoteit:
	fmtputc(f, '\'');
	while ((c = *s++) != '\0') {
		fmtputc(f, c);
		if (c == '\'')
			fmtputc(f, '\'');

	}
	fmtputc(f, '\'');
	return FALSE;
}

void initprint(void) {
	fmtinstall('A', Aconv);
	fmtinstall('L', Lconv);
	fmtinstall('S', Sconv);
	fmtinstall('T', Tconv);
	fmtinstall('D', Dconv);
#ifdef PROTECT_ENV
	fmtinstall('F', Fconv);
#else
	fmtinstall('F', fmtinstall('s', NULL));
#endif
}
