/*
 * HS (@mellvik) for the ELKS project - march 2020 
 * 
 * implements csh like command history for sash
 * 
 */

#include "sash.h"
#ifdef CMD_HISTORY

#define HISTMAX 200
#define HISTMIN 20
#define CMDBUF  80

static  int     lastcom = -1;   /* index of most recent command */
static  int     histind = 0;    /* cmd # for history list */
static  char    **histbuf;      /* array holding commands */
int     histcnt = HISTMIN;
static	void	phex(char *);
static	void	phlist();

/*#define DEBUG*/

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
        histbuf = malloc(histcnt * sizeof(char*));
        if (!histbuf) {
                fprintf(stderr, "No history buffer, history turned off\n");
                histcnt = 0;
        }
        while (k < histcnt)     /* initialize history list */
                histbuf[k++] = NULL;
}


void
do_history(argc, argv) /* list ccommands in history buffer */
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
                fprintf(stderr, "subst: %s --%s--\n", cm, (char *)(pmid+1));
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
        /*fprintf(stderr, " CMD: %s --- adr: %04x %04x %04x\n", cm, p, histbuf[lastcom]);
        phlist();*/
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

int
history(char *cmd) {
        /*
         * print command history or replace the command line with
         * the chosen command from the history list, modified by the
         * request modifier.
         * Supported: !!, !<number>, !<number>:<modifier>, history (list)
         */
        int prev_cmd, h;
        char hnum[5];
        char *cm, *rcmd = NULL;

        prev_cmd = 0;
        h = 0;
        cm = cmd;       /* save pointer for reuse */

        switch (*cmd) {
        /* todo: Add argument substitution (!$-notation) */

        case '^':
                /* do substitution on the previous command */
                if (!cmd_edit(cmd_get(-1), cmd, cm)) return(1);
                puts(cm);
                break;

        case '!':
                if (isalpha((int)*++cmd)) {
                        printf("History search not implemented.\n");
                        return(1);
                /* TODO: add search in command history here */
                }
                if (*cmd == '!')  {
                        prev_cmd = -1;          /* previous cmd */
                } else if (isdigit (*cmd) || (*cmd == '-')) {
                        hnum[h++] = *cmd++;
                        while (isdigit(*cmd) && h < 4) {  /* cmd # from history */
                                hnum[h++] = *cmd++;
                        }
                        hnum[h] = '\0';
                        prev_cmd = atoi(hnum);
                        if (map_ind(prev_cmd) < 0) {
                                printf("%d: Event not found.\n", prev_cmd);
                                return(1);
                        }
                        fflush(stderr);
			cmd--;
                }
                if (*++cmd == ':') {    /* the request has a modifier */
                        switch (*++cmd) {
                        case 'p':       /* print the selected command and make it current */
                                strcpy(cm, cmd_get(prev_cmd));
                                puts(cm);               /* echo */
                                add_to_history(cm);     /* add to history list */
                                return(1);                      /* don't execute */
                        default:
                                fputs("Illegal history modifier\n", stderr);
                                return(1); /* nothing to do */
                        }

                }
		if (strlen(cmd) > 0 ) { /* add the rest of the command line if any */
			if (!(rcmd = malloc(strlen(cmd)+1))) {
				fputs("history: malloc error\n", stderr);
				return(1);
			}
			strcpy(rcmd, cmd);
		}
                strcpy(cm, cmd_get(prev_cmd)) ;
		if (rcmd) {
			strcat(cm, rcmd);
			free(rcmd);
		}
                puts(cm);
                break;
        }      
        add_to_history(cm);
        return(0);
}

#endif /* CMD_HISTORY */

/* END CODE */
