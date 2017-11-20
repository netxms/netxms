bin_PROGRAMS = nxreportd
nxreportd_SOURCES = messages.c nxreportd.cpp
nxreportd_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxreportd_LDADD = ../../libnetxms/libnetxms.la

BUILT_SOURCES = messages.h
CLEANFILES = messages.c messages.h

messages.c: messages.h

messages.h: messages.mc
        $(PERL) @top_builddir@/tools/mc.pl messages.mc

EXTRA_DIST = \
	messages.mc messages.h messages.c \
	nxreportd.vcxproj nxreportd.vcxproj.filters \
	winsrv.cpp
