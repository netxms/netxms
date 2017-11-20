## Process this file with automake to produce Makefile.in

SUBDIRS = win32

lib_LTLIBRARIES = libnxtre.la
libnxtre_la_LDFLAGS = -no-undefined -version-info 5:0:0 $(LDFLAGS)

noinst_HEADERS =		\
	tre.h                   \
	tre-ast.h		\
	tre-compile.h		\
	tre-config.h            \
	tre-internal.h		\
	tre-match-utils.h	\
	tre-mem.h		\
	tre-parse.h		\
	tre-stack.h		\
	xmalloc.h

libnxtre_la_SOURCES =		\
	tre-ast.c		\
	tre-compile.c		\
	tre-match-approx.c	\
	tre-match-backtrack.c	\
	tre-match-parallel.c	\
	tre-mem.c		\
	tre-parse.c		\
	tre-stack.c		\
	regcomp.c		\
	regexec.c		\
	regerror.c

EXTRA_DIST = \
	AUTHORS \
	LICENSE \
	README \
	THANKS \
	Makefile.w32 \
	libtre.def \
	libtre.vcxproj libtre.vcxproj.filters \
	windll.cpp \
	xmalloc.c
