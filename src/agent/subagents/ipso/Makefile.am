lib_LTLIBRARIES = libnsm_ipso.la
libnsm_ipso_la_SOURCES = disk.cpp ipso.cpp net.cpp system.cpp
libnsm_ipso_CPPFLAGS=-I@top_srcdir@/include
libnsm_ipso_la_LDFLAGS = -lkvm
libnsm_ipso_la_LIBADD = ../../libnxagent/libnxagent.la 

EXTRA_DIST = ipso.h
