INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxsnmp.la
libnxsnmp_la_SOURCES = ber.cpp main.cpp mib.cpp oid.cpp pdu.cpp transport.cpp variable.cpp zfile.cpp
libnxsnmp_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) ../libnetxms/libnetxms.la

EXTRA_DIST = \
	libnxsnmp.dsp libnxsnmp.dsw \
	libnxsnmp.h
