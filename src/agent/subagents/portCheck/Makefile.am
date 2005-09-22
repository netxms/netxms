# $Id: Makefile.am,v 1.8 2005-09-22 05:27:57 victor Exp $

INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_portCheck.la
libnsm_portCheck_la_SOURCES = main.cpp net.cpp pop3.cpp ssh.cpp smtp.cpp \
	http.cpp custom.cpp
libnsm_portCheck_la_LDFLAGS = ../../../libnetxms/libnetxms.la ../../../libnxcscp/libnxcscp.la

EXTRA_DIST = net.h main.h
