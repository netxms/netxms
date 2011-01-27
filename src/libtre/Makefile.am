## Process this file with automake to produce Makefile.in

SUBDIRS = win32

lib_LTLIBRARIES = libtre.la
libtre_la_LDFLAGS = -no-undefined -version-info 5:0:0 $(LDFLAGS)
libtre_la_LIBADD = $(LTLIBINTL)

noinst_HEADERS =		\
	tre-ast.h		\
	tre-compile.h		\
	tre-internal.h		\
	tre-match-utils.h	\
	tre-mem.h		\
	tre-parse.h		\
	tre-stack.h		\
	xmalloc.h

libtre_la_SOURCES =		\
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
