INCLUDES=-I@top_srcdir@/include

SOURCES = main.cpp

lib_LTLIBRARIES = libnxlp.la

libnxlp_la_SOURCES = $(SOURCES)
libnxlp_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxlp_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxlp.dsp libnxlp.dsw \
	libnxlp.h
