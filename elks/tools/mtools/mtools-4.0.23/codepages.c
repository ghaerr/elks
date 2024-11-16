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
	  "����������������"
	  "����������ܢ��Pf"
	  "�����Ѫ��r������"
	  "_______________�"
	  "________________"
	  "________________"
	  "abgpSs�tftod��_N"
	  "=�<>||�~���Vn�__"
	},

	{ 819,
	  "________________"
	  "________________"
	  "����������������"
	  "����������������"
	  "����������������"
	  "����������������"
	  "����������������"
	  "����������������"
	},

	{ 850,
	  "����������������"
	  "���������������_"
	  "�����Ѫ���������"
	  "_____����____���"
	  "______��_______�"
	  "�����i���____|I_"
	  "�����յ������ޯ�"
	  "��_�����������__"
	},
	
	{ 852,
	  "�����uc�l����Z�C"
	  "�Ll��LlSs��TtL�c"
	  "����AaZzEe zCs��"
	  "_____��ES____Zz�"
	  "______Aa_______�"
	  "��D�d���e_r__TU_"
	  "���Nn�SsR�rU��t�"
	  "�~.~~�������uRr_"
	},
	
	{ 860,
	  "����������������"
	  "����������ܢ��P�"
	  "�����Ѫ��Ҭ�����"
	  "_______________�"
	  "________________"
	  "________________"
	  "abgpSs�tftod��_N"
	  "=�<>||�~���Vn�__"
	},
	
	{ 863,
	  "������������_��"
	  "����������ܢ���f"
	  "|���� ���r������"
	  "_______________�"
	  "________________"
	  "________________"
	  "abgpSs�tftod��_N"
	  "=�<>||�~���Vn�__"
	},
	
	{ 865,
	  "����������������"
	  "��������������Pf"
	  "�����Ѫ��r������"
	  "_______________�"
	  "________________"
	  "________________"
	  "abgpSs�tftod��_N"
	  "=�<>||�~���Vn�__",
	},

	/* Taiwanese (Chinese Complex Character) support */
	{ 950,
	 "����������������"
	 "����������������"
	 "����������������"
	 "����������������"
	 "����������������"
	 "����������������"
	 "����������������"
	 "����������������",
	},


	{ 0 }
};
#else
/* Should down  ISO C forbids an empty translation unit warning [-Wpedantic]: */
typedef int make_iso_compilers_happy;
#endif
