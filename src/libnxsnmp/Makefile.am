INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxsnmp.la
libnxsnmp_la_SOURCES = ber.cpp main.cpp oid.cpp pdu.cpp transport.cpp variable.cpp
libnxsnmp_la_LDFLAGS = -version-info $(LIBNXSNMP_LIBRARY_VERSION)

EXTRA_DIST = \
	libnxsnmp.dsp libnxsnmp.dsw \
	libnxsnmp.h
