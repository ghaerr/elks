# Makefile include for C86 build

ifeq "$(TOPDIR)" ""
$(error TOPDIR environment variable not set)
endif

ifeq "$(C86)" ""
$(error C86 environment variable not set)
endif

INCLUDES = -I$(TOPDIR)/libc/include -I$(TOPDIR)/elks/include -I$(TOPDIR)/libc/include/c86
DEFINES = -D__LIBC__ -D__HAS_NO_FLOATS__=1 -D__HAS_NO_LONGLONG__
LIBOBJS=$(OBJS:.o=.ocj)

CPP86FLAGS =\
    -0                          \

C86UNUSED =\
    -v                          \

C86FLAGS =\
    -g                          \
    -O                          \
    -bas86                      \
    -warn=4                     \
    -lang=c99                   \
    -align=yes                  \
    -separate=yes               \
    -stackopt=minimum           \
    -peep=all                   \
    -stackcheck=no              \

AS86FLAGS =\
    -0                          \
    -O                          \
    -j                          \
    -w-                         \

CPP=cpp86
CC=c86
AS=as86
AR=ar86

CPPFLAGS=$(CPP86FLAGS) $(INCLUDES) $(DEFINES)
CFLAGS=$(C86FLAGS)
ASFLAGS=$(AS86FLAGS)
ARFLAGS_SUB=r

%.i: %.c
	$(CPP) $(CPPFLAGS) -o $*.i $<

%.as: %.i
	$(CC) $(CFLAGS) $< $*.as

%.ocj: %.as
#	cp $*.as /tmp
	$(AS) $(ASFLAGS) -o $*.ocj $*.as
