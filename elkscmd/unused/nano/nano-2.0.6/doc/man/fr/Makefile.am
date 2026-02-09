mandir = @mandir@/fr

if USE_NANORC
man_MANS = nano.1 nanorc.5 rnano.1
BUILT_SOURCES = nano.1.html nanorc.5.html rnano.1.html
else
man_MANS = nano.1 rnano.1
BUILT_SOURCES = nano.1.html rnano.1.html
endif

nano_man_mans = nano.1 nanorc.5 rnano.1
nano_built_sources = nano.1.html nanorc.5.html rnano.1.html

nano.1.html: nano.1
	groff -t -mandoc -Thtml < $< > $@
nanorc.5.html: nanorc.5
	groff -t -mandoc -Thtml < $< > $@
rnano.1.html: rnano.1
	groff -t -mandoc -Thtml < $< > $@

EXTRA_DIST = $(nano_man_mans) $(nano_built_sources)
