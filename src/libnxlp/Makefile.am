INCLUDES=-I@top_srcdir@/include
if USE_INTERNAL_EXPAT
INCLUDES += -I@top_srcdir@/src/libexpat/libexpat
endif

SOURCES = file.cpp main.cpp parser.cpp rule.cpp

lib_LTLIBRARIES = libnxlp.la

libnxlp_la_SOURCES = $(SOURCES)
libnxlp_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxlp_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la
if USE_INTERNAL_EXPAT
libnxlp_la_LIBADD += @top_srcdir@/src/libexpat/libexpat/libnxexpat.la
endif

EXTRA_DIST = \
	libnxlp.vcproj \
	libnxlp.h \
	libnxlp.def Makefile.nw
