SOURCES = nxtux.cpp

lib_LTLIBRARIES = libnxtux.la

libnxtux_la_SOURCES = $(SOURCES)
libnxtux_la_CPPFLAGS=-I@top_srcdir@/include @TUXEDO_CPPFLAGS@ -DLIBNXTUX_EXPORTS -I@top_srcdir@/build
libnxtux_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) @TUXEDO_LDFLAGS@
libnxtux_la_LIBADD = ../../libnetxms/libnetxms.la @TUXEDO_TPINIT@ @TUXEDO_LIBS@

EXTRA_DIST = libnxtux.vcxproj libnxtux.vcxproj.filters
