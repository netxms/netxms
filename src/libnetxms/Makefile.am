INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnetxms.la
libnetxms_la_SOURCES = c_lgcc2.cpp c_lgcc3.cpp config.cpp dload.cpp hash.cpp icmp.cpp inline.cpp main.cpp md5.cpp scandir.c sha1.cpp strtoll.c strtoull.c tools.cpp unicode.cpp
libnetxms_la_LDFLAGS = -version-info $(LIBNETXMS_LIBRARY_VERSION)

EXTRA_DIST = \
	libnetxms.dsp libnetxms.dsw \
	libnetxms.vcp libnetxms.vcw \
	Makefile.nw netxms.def \
	libnetxms.h md5.h sha1.h
