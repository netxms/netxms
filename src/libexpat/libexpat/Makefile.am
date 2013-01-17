SOURCES = xmlparse.c xmlrole.c xmltok.c xmltok_impl.c xmltok_ns.c
lib_LTLIBRARIES = libnxexpat.la

libnxexpat_la_SOURCES = $(SOURCES)
libnxexpat_la_CPPFLAGS = -I@top_srcdir@/include -DHAVE_EXPAT_CONFIG_H -I..
libnxexpat_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)

EXTRA_DIST = \
	libexpat.cpp \
	amigaconfig.h \
	ascii.h \
	asciitab.h \
	expat_external.h \
	expat.h \
	iasciitab.h \
	internal.h \
	latin1tab.h \
	macconfig.h \
	nametab.h \
	utf8tab.h \
	winconfig.h \
	xmlrole.h \
	xmltok.h \
	xmltok_impl.h
