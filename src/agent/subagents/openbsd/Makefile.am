INCLUDES=-I@top_srcdir@/include
AM_CXXFLAGS=-fpermissive 

lib_LTLIBRARIES = libnsm_openbsd.la
libnsm_openbsd_la_SOURCES = openbsd.cpp system.cpp disk.cpp net.cpp
libnsm_openbsd_la_LDFLAGS = -lkvm

EXTRA_DIST = disk.h net.h system.h
