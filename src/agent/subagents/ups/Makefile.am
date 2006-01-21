INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_ups.la
libnsm_ups_la_SOURCES = apc.cpp main.cpp ups.cpp
libnsm_ups_la_LDFLAGS = $(PTHREAD_LIBS) ../../../libnetxms/libnetxms.la

EXTRA_DIST = ups.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnsm_ups.a $(libdir)/libnsm_ups.so.0 ; fi
