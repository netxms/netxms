lib_LTLIBRARIES = libnxsrv.la
libnxsrv_la_SOURCES = messages.c agent.cpp apinfo.cpp iflist.cpp isc.cpp \
                      main.cpp ndd.cpp snmp.cpp snmpproxy.cpp vlan.cpp
libnxsrv_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsrv_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsrv_la_LIBADD = ../../libnetxms/libnetxms.la ../../snmp/libnxsnmp/libnxsnmp.la

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc

EXTRA_DIST = \
	libnxsrv.vcproj libnxsrv.rc \
	messages.mc messages.c messages.h \
	libnxsrv.h resource.h
