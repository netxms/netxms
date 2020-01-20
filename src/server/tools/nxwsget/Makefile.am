bin_PROGRAMS = nxwsget
nxwsget_SOURCES = nxwsget.cpp
nxwsget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxwsget_LDFLAGS = @EXEC_LDFLAGS@
nxwsget_LDADD = \
	      ../../libnxsrv/libnxsrv.la \
	      ../../../libnxsl/libnxsl.la \
	      ../../../snmp/libnxsnmp/libnxsnmp.la \
	      ../../../libnetxms/libnetxms.la \
	      @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxwsget.vcxproj nxwsget.vcxproj.filters
