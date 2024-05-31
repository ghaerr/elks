# Makefile of /libc/math module

COMPILER ?= ia16
LIB ?= out.a

include $(TOPDIR)/libc/$(COMPILER).inc

CFLAGS += -I$(TOPDIR)/libc/math -D__BSD_VISIBLE

DOUBLE = \
	s_floor.o \
	e_rem_pio2.o \
	k_rem_pio2.o \
	s_scalbn.o \
	s_copysign.o \
	e_log.o \
	e_log10.o \
	e_exp.o \
	e_pow.o \
	e_sqrt.o \
	s_fabs.o \
	s_cos.o \
	s_sin.o \
	k_cos.o \
	k_sin.o \
	s_tan.o \
	k_tan.o \
	e_acos.o \
	e_asin.o \
	s_atan.o \
	# end of list

FLOAT = \
	s_floorf.o \
	e_rem_pio2f.o \
	s_scalbnf.o \
	s_copysignf.o \
	e_logf.o \
	e_log10f.o \
	e_expf.o \
	e_powf.o \
	e_sqrtf.o \
	s_fabsf.o \
	s_cosf.o \
	s_sinf.o \
	k_cosf.o \
	k_sinf.o \
	s_tanf.o \
	k_tanf.o \
	e_acosf.o \
	e_asinf.o \
	s_atanf.o \
	s_fminf.o \
	s_fmaxf.o \
	# end of list

OBJS = $(DOUBLE) $(FLOAT)

all: $(LIB)

$(LIB): $(LIBOBJS)
	$(RM) $@
	$(AR) $(ARFLAGS_SUB) $@ $(LIBOBJS)

clean:
	$(RM) *.[aod]
