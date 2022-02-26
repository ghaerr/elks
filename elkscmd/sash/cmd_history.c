/*
 * Helge Skrivervik (@mellvik) for the ELKS project - march 2020 
 * 
 * implements csh like command history for sash
 * 
 */

#include "sash.h"
#ifdef CMD_HISTORY

//#define DEBUG
#define HISTMAX 200
#define HISTMIN 20
#define CMDBUF  100

static  int     lastcom = -1;   /* index of most recent command */
static  int     histind = 0;    /* cmd # for history list */
static  char    **histbuf;      /* array holding commands */
int     histcnt = HISTMIN;
#ifdef DEBUG
static	void	phex(char *);
static	void	phlist(void);
#endif

void
init_hist() {
	int k = 0;
	char *cp;

        cp = getenv("HISTORY");
        if (cp) {
                histcnt = atoi(cp);
                if (histcnt > HISTMAX)
                        histcnt = HISTMAX;
        }
	if (!histcnt) return;
        histbuf = malloc(histcnt * sizeof(char*));
        if (!histbuf) {
                fprintf(stderr, "No history buffer, history turned off\n");
                histcnt = 0;
        } else {
        	while (k < histcnt)     /* initialize history list */
                	histbuf[k++] = NULL;
	}
	return;
}


void
do_history(argc, argv) /* list commands in history buffer */
	int argc;
        char **argv;
{
        int k = 0;
        int j = lastcom+1;

        while (k < histcnt) {
                if (histbuf[j])
                        printf(" %i  %s\n", histind-histcnt+k+1, histbuf[j]);
                if (++j >= histcnt)
                        j = 0;  /* wrap around */
                k++;
        }
}

int
map_ind(int idx) {      /* map history number to index into history buffer */
        int ind;

        if (idx < 0)
                ind = histind + idx + 1;        /* turn backw ref into # */
        else
                ind = idx;
        /* check sanity & return NULL if error */
        if ((ind < 0 ) || (ind > histind) || (ind < (histind - histcnt))) return(-1);
        ind = lastcom - (histind - ind);
        if (ind >=0)
                return(ind);
        else
                return(histcnt + ind);
}


char *
cmd_get(int idx) { /* return the selected command from the history buffer */
                   /* error processing already done */
        return(histbuf[map_ind(idx)]);
}

static char *
cmd_search(char * pat) {
	int idx, i;

	//printf("search '%s', histcnt %d\n", pat, histcnt);
	for (i = 0; i < histcnt; i++) {
		idx = map_ind(-i);
		//if (strstr(histbuf[idx], pat) != NULL)	/* match 'pat' anywhere in histbuf entry */
		if (!strncmp(histbuf[idx], pat, strlen(pat)))	/* match 'pat' from start of histbuf entry */
			return(histbuf[idx]);
	}
	return(NULL);
}

/* 
 * Edit the commandline in cm using substitute operation in 'edit', result pointed to by dst
 * syntax: ^old^new<EOL>
 * Return NULL on error 
 * TODO: Does not (yet) support multiple occurences of the replacement string
 * or regexp.
 */
char *
cmd_edit(char *cm, char *edit, char *dst) {
        char delim = *edit;
        char *pmid, *psub, *pend, *tmp;
	int len;

        if (!cm)
                return("\0");   /* got null string */
	len = CMDBUF + 2 + strlen(edit);
        if (!(tmp = malloc(len))) {
                printf("Malloc error in substitute.\n");
                return(NULL);
        }
	strcpy(&tmp[CMDBUF+1], edit);
	edit = &tmp[CMDBUF+1];
        *tmp = '\0';
        if ((pmid = strchr(++edit, (int) delim))) {
                *pmid = '\0';
		if ((pend = strchr((pmid+1), (int)delim))) /* third caret */
			*pend = '\0';	/* supporting blanks at the end of the subst */
                /*fprintf(stderr, "subst: %s --%s--\n", cm, (char *)(pmid+1));*/
                if (!(psub = strstr(cm, edit))) {
                        fprintf(stderr, "substitution failed\n");
                } else {
                        strncpy(tmp, cm, (int)(psub - cm));
			tmp[(int)(psub - cm)] = '\0';
                        strcat(tmp, ++pmid);
                        strncat(tmp, (char *)(psub+strlen(edit)), len - strlen(tmp) -1);
                }
        }
        strcpy(dst, tmp);
        free(tmp);
        if (*dst == '\0')
                return(NULL);
        else
                return(dst);

}

int
add_to_history(char *cm) {

        char *p;
#ifdef DEBUG
        phex(cm);
#endif
        if (++lastcom >= histcnt) lastcom = 0; /* wrap around */
        p = histbuf[lastcom];
        /*fprintf(stderr, "%s lastcom %d (%04x) p=%04x ", cm, lastcom, (int)&cm, p);*/
        if (p) free(p);
        if ((p = malloc(strlen(cm)+1)))
                strcpy(p, cm);
        else {
                p = NULL;       /* error */
                fputs("malloc error in history list\n", stderr);
                return(-1);
        }
        histbuf[lastcom] = p;
        histind++;
        //fprintf(stderr, " CMD: %s --- adr: %04x %04x %04x\n", cm, p, histbuf[lastcom]);
        //phlist();
        return(0);
}
#ifdef DEBUG
int dmap_ind(int idx) {
        int index;      /* debug wrapper for map_ind() */
        index = map_ind(idx);
        fprintf(stderr, "map_ind: in %i, out %i\n", idx, index);
        fflush(stderr);
        return(index);
}

void
phex(char *c) {
        fprintf(stderr,"len %d |", strlen(c));
        while (*c)
                fprintf(stderr, "%2x ", *c++);
        fprintf(stderr, "\n");
        fflush(stderr);
        return;
}

void
phlist() {
        int k = 0;
        while (k < histcnt) {
                fprintf(stderr, "%02i %04x %s\n", k, (int)histbuf[k], histbuf[k]);
                k++;
        }
        return;
}
#endif

static void
fixbuf(char *cmd, char *prev, char *pos, char *cm) {

	char buf[CMDBUF];

	if ((strlen(cmd) + strlen(prev) + strlen(pos)) > (CMDBUF -1)) 
		fprintf(stderr, "Buffer overflow in command history \n");
	else {
		strcpy(buf, cmd);
		strcat(buf, prev);
		strcat(buf, pos);
		strcpy(cm, buf);
	}
	return;
}

int
history(char *cmd) {
        /*
         * print command history or replace the command line with
         * the chosen command from the history list, modified by the
         * request modifier.
         * Supported: !!, !<number>, !<name>, history (list), !$
	 * all with extra text before and/or after. Add :p to print 
	 * a command and make it current w/o executing.
	 * Check bash or csh man pages for details.
         */
        int prev_cmd, h, pflag, echo;
        char hnum[10];
        char *cm, *pos, *prev;

        prev_cmd = 0;
	pflag = 0;
        h = echo = 0;
        cm = cmd;       /* save pointer for reuse */

        while (1) {

        if (*cmd == '^') {
                /* do substitution on the previous command */
                if (!cmd_edit(cmd_get(-1), cmd, cm)) return(1);
                echo++;
		break;
	}
	if ((pos = strchr(cmd, '!'))) {
		if (pos > cmd && *(pos -1) == '\\') {	/* bang is escaped, ignore */
			pos--;
			while (*pos != '\0') {
				*pos = *(pos+1); /* remove backslash */
				pos++;
			}
			break;
		}
		*pos = '\0';
		if ((prev = strchr(pos+1, ':'))) { /* find modifier, only 'p' supported */
			if (tolower((int)*(prev+1)) == 'p')
				pflag++;
			else {
				printf("Illegal history modifier\n");
				return (1);
			}
			*prev = '\0';
		}
		if (*(pos+1) == '$') {		/* add last arg from prev cmd to current */
			prev = strrchr(cmd_get(-1), ' ');
			*pos = '\0';
			fixbuf(cmd, prev, pos+2, cm);
			break;
		}
		prev = pos;
                if (isalpha((int)*(++pos))) {
			while (isalpha(*pos) && h < 10)  /* max 9 chars to search */
				hnum[h++] = *pos++;
			hnum[h] = '\0';
			if ((prev = cmd_search(hnum))) 
				fixbuf(cmd, prev, pos, cm);
			else {
				printf("Event not found\n");
				return (1);
			}
			echo++;
			break;
                }
                if (*pos == '!')  {		/* repeat last command */
			if (histind) 		/* sanity check */
				fixbuf(cmd, cmd_get(-1), pos+1, cm);
			else
				printf("Event not found\n");
			echo++;
			break;
		}
                if (isdigit (*pos) || (*pos == '-')) {
                        hnum[h++] = *pos++;
                        while (isdigit(*pos) && h < 4) {  /* cmd # from history */
                                hnum[h++] = *pos++;
                        }
                        hnum[h] = '\0';
                        prev_cmd = atoi(hnum);
                        if (map_ind(prev_cmd) < 0) {
                                printf("%d: Event not found\n", prev_cmd);
                                return(1);
                        }
			fixbuf(cmd, cmd_get(prev_cmd), pos, cm);
			echo++;
			break;
                }
        } else break;
	}
        add_to_history(cm);
	if (echo) 
		puts(cm);
	if (pflag) 
		return(1);
        return(0);
}

#endif /* CMD_HISTORY */

/* END CODE */
