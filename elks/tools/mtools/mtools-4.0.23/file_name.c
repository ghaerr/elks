/*  Copyright 1995 David C. Niemi
 *  Copyright 1996-1998,2000-2002,2008,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "vfat.h"
#include "codepage.h"
#include "file_name.h"

/* Write a DOS name + extension into a legal unix-style name.  */
char *unix_normalize (doscp_t *cp, char *ans, dos_name_t *dn, size_t ans_size)
{
	char buffer[13];
	wchar_t wbuffer[13];
	char *a;
	int j;
	
	for (a=buffer,j=0; (j<8) && (dn->base[j] > ' '); ++j,++a)
		*a = dn->base[j];
	if(dn->ext[0] > ' ') {
		*a++ = '.';
		for (j=0; j<3 && dn->ext[j] > ' '; ++j,++a)
			*a = dn->ext[j];
	}
	*a++ = '\0';
	dos_to_wchar(cp, buffer, wbuffer, 13);
	wchar_to_native(wbuffer, ans, 13, ans_size);
	return ans;
}

typedef enum Case_l {
	NONE,
	UPPER,
	LOWER
} Case_t;

static void TranslateToDos(doscp_t *toDos, const char *in, char *out,
			   size_t count, char *end, Case_t *Case, int *mangled)
{
	wchar_t buffer[12];
	wchar_t *s=buffer;
	size_t t_idx = 0;

	/* first convert to wchar, so we get to use towupper etc. */
	native_to_wchar(in, buffer, count, end, mangled);
	buffer[count]='\0';

	*Case = NONE;
	for( ;  *s ; s++) {
		/* skip spaces & dots */
		if(*s == ' ' || *s == '.') {
			*mangled |= 3;
			continue;
		}

		if (iswcntrl((wint_t)*s)) {
			/* "control" characters */
			*mangled |= 3;
			buffer[t_idx] = '_';
		} else if (iswlower((wint_t)*s)) {
			buffer[t_idx] = ch_towupper(*s);
			if(*Case == UPPER && !mtools_no_vfat)
				*mangled |= 1;
			else
				*Case = LOWER;
		} else if (iswupper((wint_t)*s)) {
			buffer[t_idx] = *s;
			if(*Case == LOWER && !mtools_no_vfat)
				*mangled |= 1;
			else
				*Case = UPPER;
		} else
			buffer[t_idx] = *s;
		t_idx++;
	}
	wchar_to_dos(toDos, buffer, out, t_idx, mangled);
}

/* dos_name
 *
 * Convert a Unix-style filename to a legal MSDOS name and extension.
 * Will truncate file and extension names, will substitute
 * the character '~' for any illegal character(s) in the name.
 */
void dos_name(doscp_t *toDos, const char *name, int verbose UNUSEDP,
	      int *mangled, dos_name_t *dn)
{
	char *s, *ext;
	size_t i;
	Case_t BaseCase, ExtCase = UPPER;

	*mangled = 0;

	/* skip drive letter */
	if (name[0] && name[1] == ':')
		name = &name[2];

	/* zap the leading path */
	name = (char *) _basename(name);
	if ((s = strrchr(name, '\\')))
		name = s + 1;
	
	memset(dn, ' ', 11);

	/* skip leading dots and spaces */
	i = strspn(name, ". ");
	if(i) {
		name += i;
		*mangled = 3;
	}

	ext = strrchr(name, '.');

	/* main name */
	TranslateToDos(toDos, name, dn->base, 8, ext, &BaseCase, mangled);
	if(ext)
		TranslateToDos(toDos, ext+1, dn->ext, 3, 0, &ExtCase,  mangled);

	if(*mangled & 2)
		autorename_short(dn, 0);

	if(!*mangled) {
		if(BaseCase == LOWER)
			*mangled |= BASECASE;
		if(ExtCase == LOWER)
			*mangled |= EXTCASE;
	}
}


/*
 * Get rid of spaces in an MSDOS 'raw' name (one that has come from the
 * directory structure) so that it can be used for regular expression
 * matching with a Unix filename.  Also used to 'unfix' a name that has
 * been altered by dos_name().
 */

wchar_t *unix_name(doscp_t *dosCp,
		   const char *base, const char *ext, char Case, wchar_t *ret)
{
	char *s, tname[9], text[4], ans[13];
	int i;

	strncpy(tname, base, 8);
	tname[8] = '\0';
	if ((s = strchr(tname, ' ')))
		*s = '\0';
	if (tname[0] == '\x05')
		tname[0] = '\xE5';

	if(!(Case & (BASECASE | EXTCASE)) && mtools_ignore_short_case)
		Case |= BASECASE | EXTCASE;

	if(Case & BASECASE)
		for(i=0;i<8 && tname[i];i++)
			tname[i] = ch_tolower(tname[i]);

	strncpy(text, ext, 3);
	text[3] = '\0';
	if ((s = strchr(text, ' ')))
		*s = '\0';

	if(Case & EXTCASE)
		for(i=0;i<3 && text[i];i++)
			text[i] = ch_tolower(text[i]);

	if (*text) {
		strcpy(ans, tname);
		strcat(ans, ".");
		strcat(ans, text);
	} else
		strcpy(ans, tname);

	/* fix special characters (above 0x80) */
	dos_to_wchar(dosCp, ans, ret, 12);
	return ret;
}

/* If null encountered, set *end to 0x40 and write nulls rest of way
 * 950820: Win95 does not like this!  It complains about bad characters.
 * So, instead: If null encountered, set *end to 0x40, write the null, and
 * write 0xff the rest of the way (that is what Win95 seems to do; hopefully
 * that will make it happy)
 */
/* Always return num */
int unicode_write(wchar_t *in, struct unicode_char *out, int num, int *end_p)
{
	int j;

	for (j=0; j<num; ++j) {
		if (*end_p)
			/* Fill with 0xff */
			out->uchar = out->lchar = 0xff;
		else {
			/* TODO / FIXME : handle case where wchat has more
			 * than 2 bytes (i.e. bytes 2 or 3 are set.
			 * ==> generate surrogate pairs?
			 */
			out->uchar = (*in & 0xffff) >> 8;
			out->lchar = *in & 0xff;
			if (! *in) {
				*end_p = VSE_LAST;
			}
		}

		++out;
		++in;
	}
	return num;
}
