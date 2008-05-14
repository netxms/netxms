# $Id$

INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_logscan.la
libnsm_logscan_la_SOURCES = logscan.cpp
libnsm_logscan_la_LDFLAGS = ../../../libnetxms/libnetxms.la

EXTRA_DIST = logscan.dsw logscan.dsp logscan.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnsm_logscan.a $(libdir)/libnsm_logscan.so.0 ; fi
