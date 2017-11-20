lib_LTLIBRARIES = libnxjava.la

libnxjava_la_SOURCES = config.cpp jre.cpp jvm.cpp main.cpp platform.cpp tools.cpp
libnxjava_la_CPPFLAGS=-I@top_srcdir@/include
libnxjava_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxjava_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la

EXTRA_DIST = \
	Makefile.w32 \
	libnxjava.vcxproj libnxjava.vcxproj.filters \
	libnxjava.h

SUBDIRS = java
