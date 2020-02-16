/*
 * Copyright (c) 1993 by David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * The "ed" built-in command (much simplified)
 */

#include "../sash.h"

#include "cmd.h"

#define	USERSIZE	1024	/* max line length typed in by user */
#define	INITBUFSIZE	1024	/* initial buffer size */

typedef	int	NUM;
typedef	int	LEN;

typedef	struct	LINE	LINE;
struct	LINE	{
	LINE	*next;
	LINE	*prev;
	LEN	len;
	char	data[1];
};

static	LINE	lines;
static	LINE	*curline;
static	NUM	curnum;
static	NUM	lastnum;
static	NUM	marks[26];
static	int	dirty;
static	char	*filename;
static	char	searchstring[USERSIZE];

static	char	*bufbase;
static	char	*bufptr;
static	LEN	bufused;
static	LEN	bufsize;


static	void	docommands();
static	void	subcommand();
static	int	getnum();
static	int	setcurnum();
static	int	initedit();
static	void	termedit();
static	void	addlines();
static	int	insertline();
static	int	deletelines();
static	int	printlines();
static	int	writelines();
static	int	readlines();
static	NUM	searchlines();
static	LEN	findstring();
static	LINE	*findline();

int
ed_main(int argc, char * argv[])
{
	if (!initedit())
		return;

	if (argc > 1) {
		filename = strdup(argv[1]);
		if (filename == NULL) {
			fprintf(stderr, "No memory\n");
			termedit();
			exit(1);
		}

		if (!readlines(filename, 1)) {
			termedit();
			exit(0);
		}

		if (lastnum)
			setcurnum(1);

		dirty = 0;
	}

	docommands();

	termedit();
	exit(0);
}

/*
 * Read commands until we are told to stop.
 */
static void
docommands()
{
	char	*cp;
	int	len;
	NUM	num1;
	NUM	num2;
	int	have1;
	int	have2;
	char	buf[USERSIZE];

	while (1) {
		printf(": ");
		fflush(stdout);

		if (fgets(buf, sizeof(buf), stdin) == NULL)
			return;

		len = strlen(buf);
		if (len == 0)
			return;

		cp = &buf[len - 1];
		if (*cp != '\n') {
			fprintf(stderr, "Command line too long\n");
			do {
				len = fgetc(stdin);
			} while ((len != EOF) && (len != '\n'));

			continue;
		}

		while ((cp > buf) && isblank(cp[-1]))
			cp--;
		*cp = '\0';

		cp = buf;
		while (isblank(*cp))
			*cp++;

		have1 = 0;
		have2 = 0;

		if ((curnum == 0) && (lastnum > 0)) {
			curnum = 1;
			curline = lines.next;
		}

		if (!getnum(&cp, &have1, &num1))
			continue;

		while (isblank(*cp))
			cp++;

		if (*cp == ',') {
			cp++;
			if (!getnum(&cp, &have2, &num2))
				continue;

			if (!have1)
				num1 = 1;

			if (!have2)
				num2 = lastnum;

			have1 = 1;
			have2 = 1;
		}

		if (!have1)
			num1 = curnum;

		if (!have2)
			num2 = num1;

		switch (*cp++) {
			case 'a':
				addlines(num1 + 1);
				break;

			case 'c':
				deletelines(num1, num2);
				addlines(num1);
				break;

			case 'd':
				deletelines(num1, num2);
				break;

			case 'f':
				if (*cp && !isblank(*cp)) {
					fprintf(stderr, "Bad file command\n");
					break;
				}

				while (isblank(*cp))
					cp++;
				if (*cp == '\0') {
					if (filename)
						printf("\"%s\"\n", filename);
					else
						printf("No filename\n");
					break;
				}

				cp = strdup(cp);
				if (cp == NULL) {
					fprintf(stderr, "No memory for filename\n");
					break;
				}

				if (filename)
					free(filename);
				filename = cp;
				break;

			case 'i':
				addlines(num1);
				break;

			case 'k':
				while (isblank(*cp))
					cp++;

				if ((*cp < 'a') || (*cp > 'a') || cp[1]) {
					fprintf(stderr, "Bad mark name\n");
					break;
				}

				marks[*cp - 'a'] = num2;
				break;

			case 'l':
				printlines(num1, num2, 1);
				break;

			case 'p':
				printlines(num1, num2, 0);
				break;

			case 'q':
				while (isblank(*cp))
					cp++;
				if (have1 || *cp) {
					fprintf(stderr, "Bad quit command\n");
					break;
				}

				if (!dirty)
					return;

				printf("Really quit? ");
				fflush(stdout);

				buf[0] = '\0';
				fgets(buf, sizeof(buf), stdin);
				cp = buf;
				while (isblank(*cp))
					cp++;
				if ((*cp == 'y') || (*cp == 'Y'))
					return;
				break;

			case 'r':
				if (*cp && !isblank(*cp)) {
					fprintf(stderr, "Bad read command\n");
					break;
				}

				while (isblank(*cp))
					cp++;
				if (*cp == '\0') {
					fprintf(stderr, "No filename\n");
					break;
				}

				if (!have1)
					num1 = lastnum;

				if (readlines(cp, num1 + 1))
					break;

				if (filename == NULL)
					filename = strdup(cp);
				break;

			case 's':
				subcommand(cp, num1, num2);
				break;

			case 'w':
				if (*cp && !isblank(*cp)) {
					fprintf(stderr, "Bad write command\n");
					break;
				}
				while (isblank(*cp))
					cp++;

				if (!have1) {
					num1 = 1;
					num2 = lastnum;
				}

				if (*cp == '\0')
					cp = filename;
				if (cp == NULL) {
					fprintf(stderr, "No file name specified\n");
					break;
				}
	
				writelines(cp, num1, num2);
				break;

			case 'z':
				switch (*cp) {
				case '-':
					printlines(curnum-21, curnum, 0);
					break;
				case '.':
					printlines(curnum-11, curnum+10, 0);
					break;
				default:
					printlines(curnum, curnum+21, 0);
					break;
				}
				break;

			case '.':
				if (have1) {
					fprintf(stderr, "No arguments allowed\n");
					break;
				}
				printlines(curnum, curnum, 0);
				break;
	
			case '-':
				if (setcurnum(curnum - 1))
					printlines(curnum, curnum, 0);
				break;

			case '=':
				printf("%d\n", num1);
				break;

			case '\0':
				if (have1) {
					printlines(num2, num2, 0);
					break;
				}

				if (setcurnum(curnum + 1))
					printlines(curnum, curnum, 0);
				break;

			default:
				fprintf(stderr, "Unimplemented command\n");
				break;
		}
	}
}


/*
 * Do the substitute command.
 * The current line is set to the last substitution done.
 */
static void
subcommand(cp, num1, num2)
	char	*cp;
	NUM	num1;
	NUM	num2;
{
	int	delim;
	char	*oldstr;
	char	*newstr;
	LEN	oldlen;
	LEN	newlen;
	LEN	deltalen;
	LEN	offset;
	LINE	*lp;
	LINE	*nlp;
	int	globalflag;
	int	printflag;
	int	didsub;
	int	needprint;

	if ((num1 < 1) || (num2 > lastnum) || (num1 > num2)) {
		fprintf(stderr, "Bad line range for substitute\n");
		return;
	}

	globalflag = 0;
	printflag = 0;
	didsub = 0;
	needprint = 0;

	if (isblank(*cp) || (*cp == '\0')) {
		fprintf(stderr, "Bad delimiter for substitute\n");
		return;
	}

	delim = *cp++;
	oldstr = cp;

	cp = strchr(cp, delim);
	if (cp == NULL) {
		fprintf(stderr, "Missing 2nd delimiter for substitute\n");
		return;
	}
	*cp++ = '\0';

	newstr = cp;
	cp = strchr(cp, delim);
	if (cp)
		*cp++ = '\0';
	else
		cp = "";

	while (*cp) switch (*cp++) {
		case 'g':
			globalflag = 1;
			break;

		case 'p':
			printflag = 1;
			break;

		default:
			fprintf(stderr, "Unknown option for substitute\n");
			return;
	}

	if (*oldstr == '\0') {
		if (searchstring[0] == '\0') {
			fprintf(stderr, "No previous search string\n");
			return;
		}
		oldstr = searchstring;
	}

	if (oldstr != searchstring)
		strcpy(searchstring, oldstr);

	lp = findline(num1);
	if (lp == NULL)
		return;

	oldlen = strlen(oldstr);
	newlen = strlen(newstr);
	deltalen = newlen - oldlen;
	offset = 0;

	while (num1 <= num2) {
		offset = findstring(lp, oldstr, oldlen, offset);
		if (offset < 0) {
			if (needprint) {
				printlines(num1, num1, 0);
				needprint = 0;
			}

			offset = 0;
			lp = lp->next;
			num1++;
			continue;
		}

		needprint = printflag;
		didsub = 1;
		dirty = 1;

		/*
		 * If the replacement string is the same size or shorter
		 * than the old string, then the substitution is easy.
		 */
		if (deltalen <= 0) {
			memcpy(&lp->data[offset], newstr, newlen);

			if (deltalen) {
				memcpy(&lp->data[offset + newlen],
					&lp->data[offset + oldlen],
					lp->len - offset - oldlen);

				lp->len += deltalen;
			}

			offset += newlen;
			if (globalflag)
				continue;

			if (needprint) {
				printlines(num1, num1, 0);
				needprint = 0;
			}

			lp = nlp->next;
			num1++;
			continue;
		}

		/*
		 * The new string is larger, so allocate a new line
		 * structure and use that.  Link it in in place of
		 * the old line structure.
		 */
		nlp = (LINE *) malloc(sizeof(LINE) + lp->len + deltalen);
		if (nlp == NULL) {
			fprintf(stderr, "Cannot get memory for line\n");
			return;
		}
		nlp->len = lp->len + deltalen;

		memcpy(nlp->data, lp->data, offset);

		memcpy(&nlp->data[offset], newstr, newlen);

		memcpy(&nlp->data[offset + newlen],
			&lp->data[offset + oldlen],
			lp->len - offset - oldlen);

		nlp->next = lp->next;
		nlp->prev = lp->prev;
		nlp->prev->next = nlp;
		nlp->next->prev = nlp;

		if (curline == lp)
			curline = nlp;

		free(lp);
		lp = nlp;

		offset += newlen;

		if (globalflag)
			continue;

		if (needprint) {
			printlines(num1, num1, 0);
			needprint = 0;
		}

		lp = lp->next;
		num1++;
	}

	if (!didsub)
		fprintf(stderr, "No substitutions found for \"%s\"\n", oldstr);
}


/*
 * Search a line for the specified string starting at the specified
 * offset in the line.  Returns the offset of the found string, or -1.
 */
static LEN
findstring(lp, str, len, offset)
	LINE	*lp;
	char	*str;
	LEN	len;
	LEN	offset;
{
	LEN	left;
	char	*cp;
	char	*ncp;

	cp = &lp->data[offset];
	left = lp->len - offset;

	while (left >= len) {
		ncp = memchr(cp, *str, left);
		if (ncp == NULL)
			return -1;

		left -= (ncp - cp);
		if (left < len)
			return -1;

		cp = ncp;
		if (memcmp(cp, str, len) == 0)
			return (cp - lp->data);

		cp++;
		left--;
	}

	return -1;
}


/*
 * Add lines which are typed in by the user.
 * The lines are inserted just before the specified line number.
 * The lines are terminated by a line containing a single dot (ugly!),
 * or by an end of file.
 */
static void
addlines(num)
	NUM	num;
{
	int	len;
	char	buf[USERSIZE + 1];

	while (fgets(buf, sizeof(buf), stdin)) {
		if ((buf[0] == '.') && (buf[1] == '\n') && (buf[2] == '\0'))
			return;

		len = strlen(buf);
		if (len == 0)
			return;

		if (buf[len - 1] != '\n') {
			fprintf(stderr, "Line too long\n");
			do {
				len = fgetc(stdin);
			} while ((len != EOF) && (len != '\n'));
			return;
		}

		if (!insertline(num++, buf, len))
			return;
	}
}


/*
 * Parse a line number argument if it is present.  This is a sum
 * or difference of numbers, '.', '$', 'x, or a search string.
 * Returns 1 if successful (whether or not there was a number). 
 * Returns 0 if there was a parsing error, with a message output.
 * Whether there was a number is returned indirectly, as is the number.
 * The character pointer which stopped the scan is also returned.
 */
static int
getnum(retcp, rethavenum, retnum)
	char	**retcp;
	int	*rethavenum;
	NUM	*retnum;
{
	char	*cp;
	char	*str;
	int	havenum;
	NUM	value;
	NUM	num;
	NUM	sign;

	cp = *retcp;
	havenum = 0;
	value = 0;
	sign = 1;

	while (1) {
		while (isblank(*cp))
			cp++;

		switch (*cp) {
			case '.':
				havenum = 1;
				num = curnum;
				cp++;
				break;

			case '$':
				havenum = 1;
				num = lastnum;
				cp++;
				break;

			case '\'':
				cp++;
				if ((*cp < 'a') || (*cp > 'z')) {
					fprintf(stderr, "Bad mark name\n");
					return 0;
				}

				havenum = 1;
				num = marks[*cp++ - 'a'];
				break;

			case '/':
				str = ++cp;
				cp = strchr(str, '/');
				if (cp)
					*cp++ = '\0';
				else
					cp = "";
				num = searchlines(str, curnum, lastnum);
				if (num == 0)
					return 0;

				havenum = 1;
				break;

			default:
				if (!isdecimal(*cp)) {
					*retcp = cp;
					*rethavenum = havenum;
					*retnum = value;
					return 1;
				}

				num = 0;
				while (isdecimal(*cp))
					num = num * 10 + *cp++ - '0';
				havenum = 1;
				break;
		}

		value += num * sign;

		while (isblank(*cp))
			cp++;

		switch (*cp) {
			case '-':
				sign = -1;
				cp++;
				break;
			case '+':
				sign = 1;
				cp++;
				break;

			default:
				*retcp = cp;
				*rethavenum = havenum;
				*retnum = value;
				return 1;
		}
	}
}


/*
 * Initialize everything for editing.
 */
static int
initedit()
{
	int	i;

	bufsize = INITBUFSIZE;
	bufbase = malloc(bufsize);
	if (bufbase == NULL) {
		fprintf(stderr, "No memory for buffer\n");
		return 0;
	}

	bufptr = bufbase;
	bufused = 0;

	lines.next = &lines;
	lines.prev = &lines;

	curline = NULL;
	curnum = 0;
	lastnum = 0;
	dirty = 0;
	filename = NULL;
	searchstring[0] = '\0';

	for (i = 0; i < 26; i++)
		marks[i] = 0;
}


/*
 * Finish editing.
 */
static void
termedit()
{
	if (bufbase)
		free(bufbase);
	bufbase = NULL;
	bufptr = NULL;
	bufsize = 0;
	bufused = 0;

	if (filename)
		free(filename);
	filename = NULL;

	searchstring[0] = '\0';

	if (lastnum)
		deletelines(1, lastnum);

	lastnum = 0;
	curnum = 0;
	curline = NULL;
}


/*
 * Read lines from a file at the specified line number.
 * Returns 1 if the file was successfully read.
 */
static int
readlines(file, num)
	char	*file;
	NUM	num;
{
	int	fd;
	int	cc;
	LEN	len;
	LEN	linecount;
	LEN	charcount;
	char	*cp;

	if ((num < 1) || (num > lastnum + 1)) {
		fprintf(stderr, "Bad line for read\n");
		return 0;
	}

	fd = open(file, 0);
	if (fd < 0) {
		perror(file);
		return 0;
	}

	bufptr = bufbase;
	bufused = 0;
	linecount = 0;
	charcount = 0;

	printf("\"%s\", ", file);
	fflush(stdout);

	do {
		cp = memchr(bufptr, '\n', bufused);
		if (cp) {
			len = (cp - bufptr) + 1;
			if (!insertline(num, bufptr, len)) {
				close(fd);
				return 0;
			}

			bufptr += len;
			bufused -= len;
			charcount += len;
			linecount++;
			num++;
			continue;
		}

		if (bufptr != bufbase) {
			memcpy(bufbase, bufptr, bufused);
			bufptr = bufbase + bufused;
		}

		if (bufused >= bufsize) {
			len = (bufsize * 3) / 2;
			cp = realloc(bufbase, len);
			if (cp == NULL) {
				fprintf(stderr, "No memory for buffer\n");
				close(fd);
				return 0;
			}

			bufbase = cp;
			bufptr = bufbase + bufused;
			bufsize = len;
		}

		cc = read(fd, bufptr, bufsize - bufused);
		bufused += cc;
		bufptr = bufbase;

	} while (cc > 0);

	if (cc < 0) {
		perror(file);
		close(fd);
		return 0;
	}

	if (bufused) {
		if (!insertline(num, bufptr, bufused)) {
			close(fd);
			return -1;
		}
		linecount++;
		charcount += bufused;
	}

	close(fd);

	printf("%d lines%s, %d chars\n", linecount,
		(bufused ? " (incomplete)" : ""), charcount);

	return 1;
}


/*
 * Write the specified lines out to the specified file.
 * Returns 1 if successful, or 0 on an error with a message output.
 */
static int
writelines(file, num1, num2)
	char	*file;
	NUM	num1;
	NUM	num2;
{
	int	fd;
	LINE	*lp;
	LEN	linecount;
	LEN	charcount;

	if ((num1 < 1) || (num2 > lastnum) || (num1 > num2)) {
		fprintf(stderr, "Bad line range for write\n");
		return 0;
	}

	linecount = 0;
	charcount = 0;

	fd = creat(file, 0666);
	if (fd < 0) {
		perror(file);
		return 0;
	}

	printf("\"%s\", ", file);
	fflush(stdout);

	lp = findline(num1);
	if (lp == NULL) {
		close(fd);
		return 0;
	}

	while (num1++ <= num2) {
		if (write(fd, lp->data, lp->len) != lp->len) {
			perror(file);
			close(fd);
			return 0;
		}

		charcount += lp->len;
		linecount++;
		lp = lp->next;
	}

	if (close(fd) < 0) {
		perror(file);
		return 0;
	}

	printf("%d lines, %d chars\n", linecount, charcount);

	return 1;
}


/*
 * Print lines in a specified range.
 * The last line printed becomes the current line.
 * If expandflag is 1, then the line is printed specially to
 * show magic characters.
 */
static int
printlines(num1, num2, expandflag)
	NUM	num1;
	NUM	num2;
	int	expandflag;
{
	LINE	*lp;
	unsigned char	*cp;
	int	ch;
	LEN	count;

	if ((num1 < 1) || (num2 > lastnum) || (num1 > num2)) {
		fprintf(stderr, "Bad line range for print\n");
		return 0;
	}

	lp = findline(num1);
	if (lp == NULL)
		return 0;

	while (num1 <= num2) {
		if (!expandflag) {
			write(STDOUT, lp->data, lp->len);
			setcurnum(num1++);
			lp = lp->next;
			continue;
		}

		/*
		 * Show control characters and characters with the
		 * high bit set specially.
		 */
		cp = lp->data;
		count = lp->len;
		if ((count > 0) && (cp[count - 1] == '\n'))
			count--;

		while (count-- > 0) {
			ch = *cp++;
			if (ch & 0x80) {
				fputs("M-", stdout);
				ch &= 0x7f;
			}
			if (ch < ' ') {
				fputc('^', stdout);
				ch += '@';
			}
			if (ch == 0x7f) {
				fputc('^', stdout);
				ch = '?';
			}
			fputc(ch, stdout);
		}
		fputs("$\n", stdout);

		setcurnum(num1++);
		lp = lp->next;
	}

	return 1;
}


/*
 * Insert a new line with the specified text.
 * The line is inserted so as to become the specified line,
 * thus pushing any existing and further lines down one.
 * The inserted line is also set to become the current line.
 * Returns 1 if successful.
 */
static int
insertline(num, data, len)
	NUM	num;
	char	*data;
	LEN	len;
{
	LINE	*newlp;
	LINE	*lp;

	if ((num < 1) || (num > lastnum + 1)) {
		fprintf(stderr, "Inserting at bad line number\n");
		return 0;
	}

	newlp = (LINE *) malloc(sizeof(LINE) + len - 1);
	if (newlp == NULL)  {
		fprintf(stderr, "Failed to allocate memory for line\n");
		return 0;
	}

	memcpy(newlp->data, data, len);
	newlp->len = len;

	if (num > lastnum)
		lp = &lines;
	else {
		lp = findline(num);
		if (lp == NULL) {
			free((char *) newlp);
			return 0;
		}
	}

	newlp->next = lp;
	newlp->prev = lp->prev;
	lp->prev->next = newlp;
	lp->prev = newlp;

	lastnum++;
	dirty = 1;

	return setcurnum(num);
}


/*
 * Delete lines from the given range.
 */
static int
deletelines(num1, num2)
	NUM	num1;
	NUM	num2;
{
	LINE	*lp;
	LINE	*nlp;
	LINE	*plp;
	NUM	count;

	if ((num1 < 1) || (num2 > lastnum) || (num1 > num2)) {
		fprintf(stderr, "Bad line numbers for delete\n");
		return 0;
	}

	lp = findline(num1);
	if (lp == NULL)
		return 0;

	if ((curnum >= num1) && (curnum <= num2)) {
		if (num2 < lastnum)
			setcurnum(num2 + 1);
		else if (num1 > 1)
			setcurnum(num1 - 1);
		else
			curnum = 0;
	}

	count = num2 - num1 + 1;

	if (curnum > num2)
		curnum -= count;

	lastnum -= count;

	while (count-- > 0) {
		nlp = lp->next;
		plp = lp->prev;
		plp->next = nlp;
		nlp->prev = plp;
		lp->next = NULL;
		lp->prev = NULL;
		lp->len = 0;
		free(lp);
		lp = nlp;
	}

	dirty = 1;

	return 1;
}


/*
 * Search for a line which contains the specified string.
 * If the string is NULL, then the previously searched for string
 * is used.  The currently searched for string is saved for future use.
 * Returns the line number which matches, or 0 if there was no match
 * with an error printed.
 */
static NUM
searchlines(str, num1, num2)
	char	*str;
	NUM	num1;
	NUM	num2;
{
	LINE	*lp;
	int	len;

	if ((num1 < 1) || (num2 > lastnum) || (num1 > num2)) {
		fprintf(stderr, "Bad line numbers for search\n");
		return 0;
	}

	if (*str == '\0') {
		if (searchstring[0] == '\0') {
			fprintf(stderr, "No previous search string\n");
			return 0;
		}
		str = searchstring;
	}

	if (str != searchstring)
		strcpy(searchstring, str);

	len = strlen(str);

	lp = findline(num1);
	if (lp == NULL)
		return 0;

	while (num1 <= num2) {
		if (findstring(lp, str, len, 0) >= 0)
			return num1;

		num1++;
		lp = lp->next;
	}

	fprintf(stderr, "Cannot find string \"%s\"\n", str);

	return 0;
}


/*
 * Return a pointer to the specified line number.
 */
static LINE *
findline(num)
	NUM	num;
{
	LINE	*lp;
	NUM	lnum;

	if ((num < 1) || (num > lastnum)) {
		fprintf(stderr, "Line number %d does not exist\n", num);
		return NULL;
	}

	if (curnum <= 0) {
		curnum = 1;
		curline = lines.next;
	}

	if (num == curnum)
		return curline;

	lp = curline;
	lnum = curnum;

	if (num < (curnum / 2)) {
		lp = lines.next;
		lnum = 1;
	}
	else if (num > ((curnum + lastnum) / 2)) {
		lp = lines.prev;
		lnum = lastnum;
	}

	while (lnum < num) {
		lp = lp->next;
		lnum++;
	}

	while (lnum > num) {
		lp = lp->prev;
		lnum--;
	}

	return lp;
}


/*
 * Set the current line number.
 * Returns 1 if successful.
 */
static int
setcurnum(num)
	NUM	num;
{
	LINE	*lp;

	lp = findline(num);
	if (lp == NULL)
		return 0;

	curnum = num;
	curline = lp;

	return 1;
}

/* END CODE */
