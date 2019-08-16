bin_PROGRAMS = nxget
nxget_SOURCES = nxget.cpp
nxget_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxget_LDFLAGS = @EXEC_LDFLAGS@
nxget_LDADD = \
	      ../../libnxsrv/libnxsrv.la \
	      ../../../libnxsl/libnxsl.la \
	      ../../../snmp/libnxsnmp/libnxsnmp.la \
	      ../../../libnetxms/libnetxms.la \
	      @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxget.vcxproj nxget.vcxproj.filters
