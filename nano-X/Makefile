all:
	@test ! -e ../elks/include/autoconf.h && echo -e "\nYou must build ELKS first.\n" && exit 1 || true
	cd src && $(MAKE)
clean:
	cd src && $(MAKE) clean
realclean:
	cd src && $(MAKE) realclean
