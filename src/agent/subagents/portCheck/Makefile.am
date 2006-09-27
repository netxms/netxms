# $Id: Makefile.am,v 1.11 2006-09-27 19:09:18 victor Exp $

INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_portCheck.la
libnsm_portCheck_la_SOURCES = main.cpp net.cpp pop3.cpp ssh.cpp smtp.cpp \
	http.cpp custom.cpp telnet.cpp
libnsm_portCheck_la_LDFLAGS = ../../../libnetxms/libnetxms.la ../../../libnxcscp/libnxcscp.la

EXTRA_DIST = net.h main.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_portCheck.so.0 ; \
		$(AR) x $(libdir)/libnsm_portCheck.a $(libdir)/libnsm_portCheck.so.0 ; \
	fi
