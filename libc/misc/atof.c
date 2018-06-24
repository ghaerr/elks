/* Copyright (C) Robert de Bath <robert@debath.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

double 
#ifdef __STDC__
atof(const char *p)
#else
atof(p)
char *p;
#endif
{
   return strtod(p, (char**)0);
}

