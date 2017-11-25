.PHONY: all clean


# ATT to AS86 conversion utility

CFLAGS=-O2 -ansi -Wpedantic -Wall -Wextra

all:: att2as86

clean::
	rm att2as86


# BINUTILS and GCC for IA16
# PREFIX must be set in environment

BINUTILS-VER=c045395ece435fcb3bf6225e9c4734b3ebcc874b
GCC-VER=049677d4411a7d54e172e57d7a20d30f73b8af94

$(BINUTILS-VER).zip:	
	wget https://github.com/crtc-demos/binutils-ia16/archive/$(BINUTILS-VER).zip

binutils.src: $(BINUTILS-VER).zip	
	unzip -q $(BINUTILS-VER).zip
	mv binutils-ia16-$(BINUTILS-VER) binutils-ia16
	touch binutils.src

$(GCC-VER).zip:
	wget https://github.com/tkchia/gcc-ia16/archive/$(GCC-VER).zip

gcc.src: $(GCC-VER).zip
	unzip -q $(GCC-VER).zip
	mv gcc-ia16-$(GCC-VER) gcc-ia16
	touch gcc.src
	
binutils.build: binutils.src
	mkdir -p binutils-build
	cd binutils-build && ../binutils-ia16/configure --target=ia16-elf --prefix="$(PREFIX)" --disable-gdb --disable-libdecnumber --disable-readline --disable-sim --disable-nls 2>&1 | tee build.log
	cd binutils-build && make 2>&1 | tee -a build.log
	cd binutils-build && make install 2>&1 | tee -a build.log
	touch binutils.build

gcc.build: gcc.src binutils.build
	mkdir -p gcc-build
	cd gcc-build && ../gcc-ia16/configure --target=ia16-elf --prefix="$(PREFIX)" --without-headers --enable-languages=c --disable-libssp --without-isl 2>&1 | tee build.log
	cd gcc-build && make 2>&1 | tee -a build.log
	cd gcc-build && make install 2>&1 | tee -a build.log
	touch gcc.build
	
all:: gcc.build binutils.build

