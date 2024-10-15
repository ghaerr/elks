/*  Copyright 1996,1997,1999,2001,2002,2008,2009 Alain Knaff.
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
#include "config.h"

#ifndef HAVE_ICONV_H
#include "codepage.h"

Codepage_t codepages[]= {
	{ 437,
	  "ÇüéâäàåçêëèïîìÄÅ"
	  "ÉæÆôöòûùÿÖÜ¢£¥Pf"
	  "áíóúñÑªº¿r¬½¼¡«»"
	  "_______________¬"
	  "________________"
	  "________________"
	  "abgpSsµtftodøØ_N"
	  "=±<>||÷~°··Vn²__"
	},

	{ 819,
	  "________________"
	  "________________"
	  " ¡¢£¤¥¦§¨©ª«¬­®¯"
	  "°±²³´µ¶·¸¹º»¼½¾¿"
	  "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ"
	  "ĞÑÒÓÔÕÖ×ØÙÚÛÜİŞß"
	  "àáâãäåæçèéêëìíîï"
	  "ğñòóôõö÷øùúûüışÿ"
	},

	{ 850,
	  "ÇüéâäàåçêëèïîìÄÅ"
	  "ÉæÆôöòûùÿÖÜø£Ø×_"
	  "áíóúñÑªº¿®¬½¼¡«»"
	  "_____ÁÂÀ©____¢¥¬"
	  "______ãÃ_______¤"
	  "ğĞÉËÈiÍÎÏ____|I_"
	  "ÓßÔÒõÕµşŞÚÙıİŞ¯´"
	  "­±_¾¶§÷¸°¨·¹³²__"
	},
	
	{ 852,
	  "ÇüéâäucçlëÕõîZÄC"
	  "ÉLlôöLlSsÖÜTtL×c"
	  "áíóúAaZzEe zCs«»"
	  "_____ÁÂES____Zz¬"
	  "______Aa_______¤"
	  "ğĞDËdÑÍÎe_r__TU_"
	  "ÓßÔNnñSsRÚrUıİt´"
	  "­~.~~§÷¸°¨·¹uRr_"
	},
	
	{ 860,
	  "ÇüéâãàåçêëèÍõìÃÂ"
	  "ÉÀÈôõòÚùÌÕÜ¢£ÙPÓ"
	  "áíóúñÑªº¿Ò¬½¼¡«»"
	  "_______________¬"
	  "________________"
	  "________________"
	  "abgpSsµtftodøØ_N"
	  "=±<>||÷~°··Vn²__"
	},
	
	{ 863,
	  "ÇüéâÂà¶çêëèïî_À§"
	  "ÉÈÊôËÏûù¤ÔÜ¢£ÙÛf"
	  "|´óú¨ ³¯Îr¬½¼¾«»"
	  "_______________¬"
	  "________________"
	  "________________"
	  "abgpSsµtftodøØ_N"
	  "=±<>||÷~°··Vn²__"
	},
	
	{ 865,
	  "ÇüéâäàåçêëèïîìÄÅ"
	  "ÉæÆôöòûùÿÖÜø£ØPf"
	  "áíóúñÑªº¿r¬½¼¡«¤"
	  "_______________¬"
	  "________________"
	  "________________"
	  "abgpSsµtftodøØ_N"
	  "=±<>||÷~°··Vn²__",
	},

	/* Taiwanese (Chinese Complex Character) support */
	{ 950,
	 "€‚ƒ„…†‡ˆ‰Š‹Œ"
	 "‘’“”•–—˜™š›œŸ"
	 " ¡¢£¤¥¦§¨©ª«¬­®¯"
	 "°±²³´µ¶·¸¹º»¼½¾¿"
	 "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ"
	 "ĞÑÒÓÔÕÖ×ØÙÚÛÜİŞß"
	 "àáâãäåæçèéêëìíîï"
	 "ğñòóôõö÷øùúûüışÿ",
	},


	{ 0 }
};
#else
/* Should down  ISO C forbids an empty translation unit warning [-Wpedantic]: */
typedef int make_iso_compilers_happy;
#endif
