# Copyright (c) 2022 TK Chia
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.

# Spec strings that control how to transform GCC command line options into
# program invocations for the actual compiler passes.  See the gcc & gccint
# info files.

# This is used by %(link), defined below.
*cmodel_long_ld:
%{mcmodel=*:%*.ld;:small.ld}

# Define this spec string to glom onto any runtime-specific -m... switches
# that are recognized & handled by this specs file.  Any runtime-specific
# switches that are not covered by this spec string will result in an error.
*ia16_impl_rt_switches:
%{maout} %{maout-total=*} %{maout-chmem=*} %{maout-stack=*} %{maout-heap=*} %{maout-symtab}

# Transformations and checks to apply to the front-end's own command line
# (DRIVER_SELF_SPECS).
*self_spec:
-nostdinc \
%{mno-segelf:;:-msegelf} \
%{fuse-ld=*:;:-fuse-ld=gold} \
%{mno-protected-mode:;:-mprotected-mode} \
%{mcmodel=*:;:-mcmodel=small} \
%{maout-total=*:\
  %{maout-chmem=*:%emay not use both -maout-total= and -maout-chmem=}\
  %{maout-stack=*:%emay not use both -maout-total= and -maout-stack=}\
  %{maout-heap=*:%emay not use both -maout-total= and -maout-heap=}} \
%{maout-chmem=*:\
  %{maout-stack=*:%emay not use both -maout-chmem= and -maout-stack=}\
  %{maout-heap=*:%emay not use both -maout-chmem= and -maout-heap=}}

# Assembler options.
*asm:
%{msegelf:--32-segelf}

# C preprocessor options.
#
# 1) Bruce's C compiler defines either __MSDOS__ or __ELKS__; follow it here.
#
# 2) The -isystem stuff is a hack.  When -mr=elks is specified, then, combined
#    with the -nostdinc above, this hack will (try to) make GCC use the
#    include files under the -mr=elks multilib directory, rather than the
#    Newlib include files in the usual locations.
#
#     We also need to extend the hack to
#	* rope in the libgcc include directories --- via `include-fixed' ---
#	  since elks-libc's headers use libgcc's;
#	* fall back on the include directories for the default calling
#	  convention (.../ia16-elf/lib/elkslibc/include/), in case the user
#	  says e.g. `-mr=elks -mregparmcall' and no include files are
#	  installed specifically for this calling convention.
#
#     Again, this is a hack.  -- tkchia
*cpp:
+ \
-D__ELKS__ \
-isystem include-fixed/../include%s \
-isystem include-fixed%s \
-isystem include%s \
-isystem elkslibc/include%s

# Linker options.
*link:
%{!T*:%Telks-%(cmodel_long_ld)} \
-m i386elks \
%{maout-total=*:--defsym=_total=%*} \
%{maout-chmem=*:--defsym=_chmem=%*} \
%{maout-stack=*:--defsym=_stack=%*} \
%{maout-heap=*:--defsym=_heap=%*}

# Object file to place at start of link.
*startfile:
-l:crt0.o

# Additional step(s) to run after link (POST_LINK_SPEC).
*post_link:
%{!mno-post-link:\
  %:if-exists-else(../lib/../bin/elf2elks%s elf2elks) \
  %{v} \
  %{mcmodel=tiny:--tiny} \
  %{maout-total=*:--total-data %*} \
  %{maout-chmem=*:--chmem %*} \
  %{maout-stack=*:--stack %*} \
  %{maout-heap=*:--heap %*} \
  %{maout-symtab:--symtab} \
  %{o*:%*} %{!o*:a.out}}
