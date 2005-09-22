INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_ping.la
libnsm_ping_la_SOURCES = ping.cpp
libnsm_ping_la_LDFLAGS = $(PTHREAD_LIBS) ../../../libnetxms/libnetxms.la

EXTRA_DIST = ping.h ping.def Makefile.nw
