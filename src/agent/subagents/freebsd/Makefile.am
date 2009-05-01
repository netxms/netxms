INCLUDES=-I@top_srcdir@/include
#AM_CXXFLAGS=-fpermissive 

lib_LTLIBRARIES = libnsm_freebsd.la
libnsm_freebsd_la_SOURCES = freebsd.cpp system.cpp disk.cpp net.cpp proc.cpp
libnsm_freebsd_la_LDFLAGS = -lkvm

EXTRA_DIST = disk.h net.h system.h
