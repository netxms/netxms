bin_PROGRAMS = nxsnmpset
nxsnmpset_SOURCES = nxsnmpset.cpp
nxsnmpset_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxsnmpset_LDFLAGS = @EXEC_LDFLAGS@
nxsnmpset_LDADD = ../../libnetxms/libnetxms.la ../libnxsnmp/libnxsnmp.la @EXEC_LIBS@

if USE_INTERNAL_JANSSON
nxsnmpset_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxsnmpset_LDADD += -ljansson
endif

EXTRA_DIST = \
	nxsnmpset.vcxproj nxsnmpset.vcxproj.filters
