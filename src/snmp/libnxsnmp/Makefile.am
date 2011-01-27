INCLUDES=-I@top_srcdir@/include

SOURCES = ber.cpp engine.cpp main.cpp mib.cpp oid.cpp pdu.cpp \
          security.cpp transport.cpp variable.cpp zfile.cpp

lib_LTLIBRARIES = libnxsnmp.la

libnxsnmp_la_SOURCES = $(SOURCES)
libnxsnmp_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsnmp_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxsnmp.vcproj libnxsnmpw.vcproj \
	libnxsnmp.h
