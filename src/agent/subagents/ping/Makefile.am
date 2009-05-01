INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_ping.la
libnsm_ping_la_SOURCES = ping.cpp
libnsm_ping_la_LDFLAGS = ../../../libnetxms/libnetxms.la

EXTRA_DIST = ping.h ping.vcproj ping.def Makefile.nw

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_ping.so.0 ; \
		$(AR) x $(libdir)/libnsm_ping.a $(libdir)/libnsm_ping.so.0 ; \
	fi
