INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxsrv.la
libnxsrv_la_SOURCES = agent.cpp main.cpp rwlock.cpp
libnxsrv_la_LDFLAGS = -version-info $(LIBNXSRV_LIBRARY_VERSION)

EXTRA_DIST = \
	libnxsrv.dsp libnxsrv.dsw \
	libnxsrv.h
