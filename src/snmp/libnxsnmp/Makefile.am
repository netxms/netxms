SOURCES = ber.cpp main.cpp mib.cpp oid.cpp pdu.cpp \
          scan.cpp security.cpp snapshot.cpp transport.cpp util.cpp \
          variable.cpp zfile.cpp

lib_LTLIBRARIES = libnxsnmp.la

libnxsnmp_la_SOURCES = $(SOURCES)
libnxsnmp_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build -DLIBNXSNMP_EXPORTS
libnxsnmp_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsnmp_la_LIBADD = ../../libnetxms/libnetxms.la

if USE_INTERNAL_JANSSON
libnxsnmp_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
libnxsnmp_la_LIBADD += -ljansson
endif


EXTRA_DIST = \
	libnxsnmp.vcxproj libnxsnmp.vcxproj.filters \
	libnxsnmp.h
