SOURCES = extractor.cpp

lib_LTLIBRARIES = libnxsde.la

libnxsde_la_SOURCES = $(SOURCES)
libnxsde_la_CPPFLAGS =-I@top_srcdir@/include -I@top_srcdir@/build -DLIBNXSDE_EXPORTS @LIBJQ_CPPFLAGS@
libnxsde_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) @LIBJQ_LDFLAGS@
libnxsde_la_LIBADD = ../../libnetxms/libnetxms.la 

if USE_LIBJQ
libnxsde_la_LIBADD += -ljq
endif

EXTRA_DIST = libnxsde.h windll.cpp
