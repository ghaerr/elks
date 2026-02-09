# Port of ncurses-5.2 to ELKS by Greg Haerr, Mar 2021

./configure --disable-big-core --enable-getcap --disable-database --without-default-terminfo-dir --disable-termcap --with-fallbacks=ansi --without-progs --without-debug --without-ada --without-cxx-binding --without-curses-h --without-termlib --disable-ext-funcs

# --disable-ext-funcs

# fixes
# INFINITY 32000, fixes v2.0.6 xenl bug
# remove float from lib_mvcur.c cost calcs
# fallback.c edits:
#	xenl TRUE, just in case to force better cursor moves
#	remove ich and rep, ELKS console doesn't implement
# xterm-256color resets xenl on any ESC sequence, including \E[m
#	ELKS console resets on cursor move, but not \E[G or \E[d (quick moves)
# ELKS console added:
#	\E[d	vpa
#	\E[G	hpa
#	\E[J	clear screen to bottom
#	\E[K	clear line to EOL
#	\E[1K	clear line to BOL
#	\E[2K	clear line
#	\E[p1;p2;p3m	varargs ansi color/mode
