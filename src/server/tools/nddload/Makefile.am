bin_PROGRAMS = nddload
nddload_SOURCES = nddload.cpp
nddload_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nddload_LDFLAGS = @EXEC_LDFLAGS@
nddload_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la ../../../snmp/libnxsnmp/libnxsnmp.la \
	      @SERVER_LIBS@ @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nddload.vcxproj nddload.vcxproj.filters
