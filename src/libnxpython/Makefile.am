SOURCES = extensions.cpp main.cpp manager.cpp

lib_LTLIBRARIES = libnxpython.la

libnxpython_la_SOURCES = $(SOURCES)
libnxpython_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXPYTHON_EXPORTS @PYTHON_CPPFLAGS@
libnxpython_la_LDFLAGS = -release $(NETXMS_LIBRARY_VERSION) @PYTHON_LDFLAGS@
libnxpython_la_LIBADD = @top_srcdir@/src/libnetxms/libnetxms.la @PYTHON_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	libnxpython.vcxproj libnxpython.vcxproj.filters \
	libnxpython.h
