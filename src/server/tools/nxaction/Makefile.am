bin_PROGRAMS = nxaction
nxaction_SOURCES = nxaction.cpp
nxaction_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxaction_LDFLAGS = @EXEC_LDFLAGS@
nxaction_LDADD = \
		 ../../libnxsrv/libnxsrv.la \
		 ../../../libnxsl/libnxsl.la \
		 ../../../snmp/libnxsnmp/libnxsnmp.la \
		 ../../../libnetxms/libnetxms.la \
		 @EXEC_LIBS@

EXTRA_DIST = \
	nxaction.vcxproj nxaction.vcxproj.filters
