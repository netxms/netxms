# $Id: Makefile.am,v 1.1 2006-03-09 11:14:55 alk Exp $

INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_logscan.la
libnsm_logscan_la_SOURCES = logscan.cpp
libnsm_logscan_la_LDFLAGS = ../../../libnetxms/libnetxms.la ../../../libnxcscp/libnxcscp.la

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then $(AR) x $(libdir)/libnsm_logscan.a $(libdir)/libnsm_logscan.so.0 ; fi
