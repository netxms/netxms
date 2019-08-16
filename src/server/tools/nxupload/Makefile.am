bin_PROGRAMS = nxupload
nxupload_SOURCES = nxupload.cpp
nxupload_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxupload_LDFLAGS = @EXEC_LDFLAGS@
nxupload_LDADD = \
		 ../../libnxsrv/libnxsrv.la \
		 ../../../libnxsl/libnxsl.la \
		 ../../../snmp/libnxsnmp/libnxsnmp.la \
		 ../../../libnetxms/libnetxms.la \
		 @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxupload.vcxproj nxupload.vcxproj.filters
