INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnetxms.la
libnetxms_la_SOURCES = config.cpp dload.cpp hash.cpp icmp.cpp inline.cpp main.cpp md5.cpp scandir.c sha1.cpp strtoll.c strtoull.c threads.cpp tools.cpp unicode.cpp
libnetxms_la_LDFLAGS = -version-info $(LIBNETXMS_LIBRARY_VERSION) $(PTHREAD_LIBS)

EXTRA_DIST = \
	libnetxms.dsp libnetxms.dsw \
	libnetxms.vcp libnetxms.vcw \
	Makefile.nw netxms.def \
	libnetxms.h md5.h sha1.h \
	regex.c c_lgcc3.cpp
