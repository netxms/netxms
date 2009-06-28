INCLUDES=-I@top_srcdir@/include

SOURCES = ber.cpp engine.cpp main.cpp mib.cpp oid.cpp pdu.cpp \
          security.cpp transport.cpp variable.cpp zfile.cpp

if BUILD_UNICODE_LIBS
lib_LTLIBRARIES = libnxsnmp.la libnxsnmpw.la
else
lib_LTLIBRARIES = libnxsnmp.la
endif

libnxsnmp_la_SOURCES = $(SOURCES)
libnxsnmp_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsnmp_la_LIBADD = ../../libnetxms/libnetxms.la

libnxsnmpw_la_SOURCES = $(SOURCES)
libnxsnmpw_la_CPPFLAGS = -DUNICODE
libnxsnmpw_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsnmpw_la_LIBADD = ../../libnetxms/libnetxmsw.la

EXTRA_DIST = \
	libnxsnmp.vcproj libnxsnmpw.vcproj \
	libnxsnmp.h
