/*  Copyright 1996,1997,2001,2002,2009 Alain Knaff.
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

typedef struct Codepage_l {
	int nr;   
	unsigned char tounix[128];
} Codepage_t;


typedef struct country_l {
	int country;
	int codepage;
	int default_codepage;
	int to_upper;
} country_t;


void init_codepage(void);
unsigned char to_dos(unsigned char c);
void to_unix(char *a, int n);
char contents_to_unix(char a);

extern Codepage_t *Codepage;
extern char *mstoupper;
extern country_t countries[];
extern unsigned char toucase[][128];
extern Codepage_t codepages[];
extern char *country_string;
