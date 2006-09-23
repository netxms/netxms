INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxsms_generic.la
libnxsms_generic_la_SOURCES = main.cpp
libnxsms_generic_la_LDFLAGS = -version-info $(SMSDRV_LIBRARY_VERSION) $(PTHREAD_LIBS)
libnxsms_generic_la_LIBADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la

EXTRA_DIST = \
	generic.dsp generic.dsw \
	main.h
