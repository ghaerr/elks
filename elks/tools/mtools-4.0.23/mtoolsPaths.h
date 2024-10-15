/*  Copyright 1997,2001,2002,2009 Alain Knaff.
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
 * Paths of the configuration files.
 * This file may be changed by the user as needed.
 * There are three empty lines between each definition.
 * These ensure that "local" patches and official patches have
 * only a very low probability of conflicting.
 */


#define CONF_FILE "/etc/mtools.conf"


#define OLD_CONF_FILE "/etc/mtools"



#define LOCAL_CONF_FILE "/etc/default/mtools.conf"
/* Use this if you like to keep the configuration file in a non-standard
 * place such as /etc/default, /opt/etc, /usr/etc, /usr/local/etc ...
 */

#define SYS_CONF_FILE SYSCONFDIR "/mtools.conf"

#define OLD_LOCAL_CONF_FILE "/etc/default/mtools"



#define CFG_FILE1 "/.mtoolsrc"



/* END */
