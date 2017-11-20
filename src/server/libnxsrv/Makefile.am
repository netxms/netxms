lib_LTLIBRARIES = libnxsrv.la
libnxsrv_la_SOURCES = messages.c agent.cpp apinfo.cpp hdlink.cpp iflist.cpp isc.cpp \
                      main.cpp ndd.cpp snmpproxy.cpp vlan.cpp
libnxsrv_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include
libnxsrv_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsrv_la_LIBADD = \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnetxms/libnetxms.la

if USE_INTERNAL_JANSSON
libnxsrv_la_LIBADD += @top_srcdir@/src/jansson/libnxjansson.la
else
libnxsrv_la_LIBADD += -ljansson
endif

messages.c: messages.h

messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc

EXTRA_DIST = \
	libnxsrv.vcxproj libnxsrv.vcxproj.filters libnxsrv.rc \
	messages.mc messages.c messages.h \
	libnxsrv.h resource.h
