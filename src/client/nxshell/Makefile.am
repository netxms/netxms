bin_PROGRAMS = nxshell
nxshell_SOURCES = nxshell.cpp
nxshell_CPPFLAGS = -I@top_srcdir@/include
nxshell_LDADD = @top_srcdir@/src/libnxjava/libnxjava.la @top_srcdir@/src/libnetxms/libnetxms.la

EXTRA_DIST = Makefile.w32 nxshell.vcxproj nxshell.vcxproj.filters

SUBDIRS = java
