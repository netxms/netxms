INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_logwatch.la
libnsm_logwatch_la_SOURCES = main.cpp
libnsm_logwatch_la_LDFLAGS = ../../../libnetxms/libnetxms.la ../../../libnxlp/libnxlp.la

EXTRA_DIST = logwatch.h logwatch.dsp logwatch.dsw logwatch.def Makefile.nw \
             eventlog.cpp

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_logwatch.so.0 ; \
		$(AR) x $(libdir)/libnsm_logwatch.a $(libdir)/libnsm_logwatch.so.0 ; \
	fi
