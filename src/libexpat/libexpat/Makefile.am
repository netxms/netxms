SOURCES = xmlparse.c xmlrole.c xmltok.c xmltok_impl.c xmltok_ns.c
lib_LTLIBRARIES = libnxexpat.la

libnxexpat_la_SOURCES = $(SOURCES)
libnxexpat_la_CPPFLAGS = -I@top_srcdir@/include -DHAVE_EXPAT_CONFIG_H -DXML_BUILDING_EXPAT -I..
libnxexpat_la_LDFLAGS = -version-info 2:0:0

EXTRA_DIST = \
	libexpat.vcxproj libexpat.vcxproj.filters \
	libexpat.cpp \
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
