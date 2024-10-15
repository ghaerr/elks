/*  Copyright 2018 Alain Knaff.
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
 *
 */
#include "sysincludes.h"
#include "mtools.h"

unsigned int strtoui(const char *nptr, char **endptr, int base) {
    unsigned long l = strtoul(nptr, endptr, base);
    if(l > UINT_MAX) {
	fprintf(stderr, "%lu too large\n", l);
	exit(1);
    }
    return (unsigned int) l;
}

unsigned int atoui(const char *str) {
    return strtoui(str, 0, 0);
}

int strtoi(const char *nptr, char **endptr, int base) {
    long l = strtol(nptr, endptr, base);
    if(l > INT_MAX || l < INT_MIN) {
	fprintf(stderr, "%lu out ob bounds\n", l);
	exit(1);
    }
    return (int) l;
}

unsigned long atoul(const char *str) {
    return strtoul(str, 0, 0);
}

uint8_t strtou8(const char *nptr, char **endptr, int base) {
    unsigned long l = strtoul(nptr, endptr, base);
    if(l > UINT8_MAX) {
	fprintf(stderr, "%lu too large\n", l);
	exit(1);
    }
    return (uint8_t) l;
}

uint8_t atou8(const char *str) {
    return strtou8(str, 0, 0);
}

uint16_t strtou16(const char *nptr, char **endptr, int base) {
    unsigned long l = strtoul(nptr, endptr, base);
    if(l > UINT16_MAX) {
	fprintf(stderr, "%lu too large\n", l);
	exit(1);
    }
    return (uint16_t) l;
}

uint16_t atou16(const char *str) {
    return strtou16(str, 0, 0);
}

uint32_t strtou32(const char *nptr, char **endptr, int base) {
    unsigned long l = strtoul(nptr, endptr, base);
    if(l > UINT32_MAX) {
	fprintf(stderr, "%lu too large\n", l);
	exit(1);
    }
    return (uint32_t) l;
}

uint32_t atou32(const char *str) {
    return strtou32(str, 0, 0);
}
