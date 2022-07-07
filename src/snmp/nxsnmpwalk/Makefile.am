bin_PROGRAMS = nxsnmpwalk
nxsnmpwalk_SOURCES = nxsnmpwalk.cpp
nxsnmpwalk_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxsnmpwalk_LDFLAGS = @EXEC_LDFLAGS@
nxsnmpwalk_LDADD = ../../libnetxms/libnetxms.la ../libnxsnmp/libnxsnmp.la @EXEC_LIBS@

if USE_INTERNAL_JANSSON
nxsnmpwalk_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxsnmpwalk_LDADD += -ljansson
endif

EXTRA_DIST = \
	nxsnmpwalk.vcxproj nxsnmpwalk.vcxproj.filters
