%define _binary_payload w9.gzdio
Name:           mtools
Summary:        mtools, read/write/list/format DOS disks under Unix
Version:        4.0.23
Release:        1
License:        GPLv3+
Group:          Utilities/System
URL:            http://www.gnu.org/software/mtools/
Source:         ftp://ftp.gnu.org/gnu/%{name}/%{name}-%{version}.tar.gz
Buildroot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)


%description
Mtools is a collection of utilities to access MS-DOS disks from GNU
and Unix without mounting them. It supports long file names, OS/2 Xdf
disks, ZIP/JAZ disks and 2m disks (store up to 1992k on a high density
3 1/2 disk).


%prep
%setup -q

./configure \
    --prefix=%{buildroot}%{_prefix} \
    --sysconfdir=/etc \
    --infodir=%{buildroot}%{_infodir} \
    --mandir=%{buildroot}%{_mandir} \
    --enable-floppyd \

%build
make

%clean
echo rm -rf $RPM_BUILD_ROOT
[ X%{buildroot} != X ] && [ X%{buildroot} != X/ ] && rm -fr %{buildroot}

%install
make install
make install-info
strip %{buildroot}%{_bindir}/mtools %{buildroot}%{_bindir}/mkmanifest %{buildroot}%{_bindir}/floppyd
rm %{buildroot}%{_infodir}/dir

%files
%defattr(-,root,root)
%{_infodir}/mtools.info*
%{_mandir}/man1/floppyd.1*
%{_mandir}/man1/floppyd_installtest.1.gz
%{_mandir}/man1/mattrib.1*
%{_mandir}/man1/mbadblocks.1*
%{_mandir}/man1/mcat.1*
%{_mandir}/man1/mcd.1*
%{_mandir}/man1/mclasserase.1*
%{_mandir}/man1/mcopy.1*
%{_mandir}/man1/mdel.1*
%{_mandir}/man1/mdeltree.1*
%{_mandir}/man1/mdir.1*
%{_mandir}/man1/mdu.1*
%{_mandir}/man1/mformat.1*
%{_mandir}/man1/minfo.1*
%{_mandir}/man1/mkmanifest.1*
%{_mandir}/man1/mlabel.1*
%{_mandir}/man1/mmd.1*
%{_mandir}/man1/mmount.1*
%{_mandir}/man1/mmove.1*
%{_mandir}/man1/mpartition.1*
%{_mandir}/man1/mrd.1*
%{_mandir}/man1/mren.1*
%{_mandir}/man1/mshortname.1*
%{_mandir}/man1/mshowfat.1*
%{_mandir}/man1/mtools.1*
%{_mandir}/man5/mtools.5*
%{_mandir}/man1/mtoolstest.1*
%{_mandir}/man1/mtype.1*
%{_mandir}/man1/mzip.1*
%{_bindir}/amuFormat.sh
%{_bindir}/mattrib
%{_bindir}/mbadblocks
%{_bindir}/mcat
%{_bindir}/mcd
%{_bindir}/mclasserase
%{_bindir}/mcopy
%{_bindir}/mdel
%{_bindir}/mdeltree
%{_bindir}/mdir
%{_bindir}/mdu
%{_bindir}/mformat
%{_bindir}/minfo
%{_bindir}/mkmanifest
%{_bindir}/mlabel
%{_bindir}/mmd
%{_bindir}/mmount
%{_bindir}/mmove
%{_bindir}/mpartition
%{_bindir}/mrd
%{_bindir}/mren
%{_bindir}/mshortname
%{_bindir}/mshowfat
%{_bindir}/mtools
%{_bindir}/mtoolstest
%{_bindir}/mtype
%{_bindir}/mzip
%{_bindir}/floppyd
%{_bindir}/floppyd_installtest
%{_bindir}/mcheck
%{_bindir}/mcomp
%{_bindir}/mxtar
%{_bindir}/tgz
%{_bindir}/uz
%{_bindir}/lz
%doc NEWS

%pre
groupadd floppy 2>/dev/null || echo -n ""

%post
if [ -f %{_bindir}/install-info ] ; then
	if [ -f %{_infodir}/dir ] ; then
		%{_bindir}/install-info %{_infodir}/mtools.info %{_infodir}/dir
	fi
	if [ -f %{_infodir}/dir.info ] ; then
		%{_bindir}/install-info %{_infodir}/mtools.info %{_infodir}/dir.info
	fi
fi


%preun
install-info --delete %{_infodir}/mtools.info %{_infodir}/dir.info
if [ -f %{_bindir}/install-info ] ; then
	if [ -f %{_infodir}/dir ] ; then
		%{_bindir}/install-info --delete %{_infodir}/mtools.info %{_infodir}/dir
	fi
	if [ -f %{_infodir}/dir.info ] ; then
		%{_bindir}/install-info --delete %{_infodir}/mtools.info %{_infodir}/dir.info
	fi
fi

%changelog
* Sun Dec 09 2018 Alain Knaff <alain@knaff.lu>
- Address lots of compiler warnings (assignments between different types)
- Network speedup fixes for floppyd (TCP_CORK)
- Typo fixes
- Explicitly pass available target buffer size for character set conversions
* Sun Dec 02 2018 Alain Knaff <alain@knaff.lu>
- Fixed -f flag for mformat (size is KBytes, rather than sectors)
- Fixed toupper/tolower usage (unsigned char rather than plain signed)
* Sat Nov 24 2018 Alain Knaff <alain@knaff.lu>
- Fixed compilation for MingW
- After MingW compilation, make sure executable has .exe extension
- Addressed compiler warnings
- Fixed length handling in character set conversion (Unicode file names)
- Fixed matching of character range, when containing Unicode characters (mdir "c:test[α-ω].exe")
- Fixed initialization of my_scsi_cmd constructor
* Sun Nov 11 2018 Alain Knaff <alain@knaff.lu>
- initialize directory entries to 0
- bad message "Too few sectors" replaced with "Too many sectors"
- apostrophe in mlabel no longer causes generation of long entry
- option to fake system date for file creation using the SOURCE_DATE_EPOCH environment variables
- can now be compiled with "clang" compiler
- fallback function for strndup, for those platforms that do not have it
- fixed a number of -Wextra warnings
- new compressed archive formats for uz/lz
- allow to specify number of reserved sectors for FAT32.
- file/device locking with timeout (rather than immediate failure)
- fixed support for BPB-less legacy formats.
- removed check that disk must be an integer number of tracks.
- removed .eh/.oh macros from manual pages
* Sat Sep 29 2018 Alain Knaff <alain@knaff.lu>
- Fix for short file names starting with character 0xE5	(by remapping it to 0x5)
- mpartition: Partition types closer to what Microsoft uses
- mformat: figure out LBA geometry as last resort if geometry
is neither specified in config and/or commandline, nor can be
queried from the device
- mformat: use same default cluster size by size as Microsoft for FAT32
- additional sanity checks
- document how cluster size is picked in mformat.c man page
- document how partition types are picked in mpartition.c man page
* Wed Jan 09 2013 Alain Knaff <alain@knaff.lu>
- Fix for names of iconv encodings on AIX
- Fix mt_size_t on NetBSD
- Fixed compilation on Mingw
- Fixed doc (especially mformat)
- Fix mformating of FAT12 filesystems with huge cluster sizes
- Minfo prints image file name in mformat command line if an image
- file name was given
- Always generate gzip-compressed RPMs, in order to remain
- compatible with older distributions
- Fixed buffer overflow with drive letter in mclasserase
* Wed Jun 29 2011 Alain Knaff <alain@knaff.lu>
- mbadblocks now takes a list of bad blocks (either as sectors
  or as clusters)
- mbadblocks now is able to do write scanning for bad blocks
- mshowfat can show cluster of specific offset
- Enable mtools to deal with very small sector sizes...
- Fixed encoding of all-lowercase names (no need to mangle
  these)
- Consider every directory entry after an ENDMARK (0x00) to be deleted
- After writing a new entry at end of a directory, be sure to also add
  an ENDMARK (0x00)
- Deal with possibility of a NULL pointer being returned by
  localtime during timestamp conversion
* Sat Apr 16 2011 Alain Knaff <alain@knaff.lu>
- configure.in fixes
- fixed formatting of fat_size_calculation.tex document
- compatibility with current autoconfig versions
- Make it clear that label is limited to 11 characters
- Fixed typo in initialization of FAT32 info sector
* Sun Oct 17 2010 Alain Knaff <alain@knaff.lu>
- Added missing -i option to mshortname
* Sun Oct 17 2010 Alain Knaff <alain@knaff.lu>
- Released v4_0_14:
- New mshortname command
- Fix floppyd for disks bigger than 2 Gig
- Remove obsolete -z flag
- Remove now unsupported AC_USE_SYSTEM_EXTENSIONS
- Fixed output formatting of mdir if MTOOLS_DOTTED_DIR is set
- Mformat now correctly writes backup boot sector
- Fixed signedness of serial number in mlabel
- Fixed buffer size problem in mlabel
- Make mlabel write backup boot sector if FAT32
- Catch situation where both clear and new label are given to mlabel
- Quote filename parameters to scripts
- Mformat: Close file descriptor for boot sector
- Added lzip support to scripts/uz
- Added Tot_sectors option to mformat
- Fixed hidden sector handling in mformat
- Minfo generates mformat command lines containing new -T option
- Mlabel prints error if label too long
* Sun Feb 28 2010 Alain Knaff <alain@knaff.lu>
- Merged Debian patches
* Tue Nov 03 2009 Alain Knaff <alain@knaff.lu>
- Mingw compatibility fixes
