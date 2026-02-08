bin_PROGRAMS = nxmibdump
nxmibdump_SOURCES = nxmibdump.cpp
nxmibdump_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/snmp/libnxsnmp -I@top_srcdir@/build
nxmibdump_LDFLAGS = @EXEC_LDFLAGS@
nxmibdump_LDADD = ../../libnetxms/libnetxms.la ../libnxsnmp/libnxsnmp.la @EXEC_LIBS@

if USE_INTERNAL_JANSSON
nxmibdump_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxmibdump_LDADD += -ljansson
endif

EXTRA_DIST = \
	nxmibdump.vcxproj nxmibdump.vcxproj.filters
