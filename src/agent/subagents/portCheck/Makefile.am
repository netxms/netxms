# $Id$

INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_portCheck.la
libnsm_portCheck_la_SOURCES = main.cpp net.cpp pop3.cpp ssh.cpp smtp.cpp \
	http.cpp custom.cpp telnet.cpp
libnsm_portCheck_la_LDFLAGS = ../../../libnetxms/libnetxms.la

EXTRA_DIST = portcheck.vcproj net.h main.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_portCheck.so.0 ; \
		$(AR) x $(libdir)/libnsm_portCheck.a $(libdir)/libnsm_portCheck.so.0 ; \
	fi
