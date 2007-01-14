INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/server/include

lib_LTLIBRARIES = libnxsrv.la
libnxsrv_la_SOURCES = messages.c agent.cpp db.cpp log.cpp main.cpp snmpproxy.cpp
libnxsrv_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxsrv_la_LIBADD = ../../libnetxms/libnetxms.la ../../libnxsnmp/libnxsnmp.la

messages.c: ../include/messages.h

../include/messages.h: messages.mc
	$(PERL) @top_builddir@/tools/mc.pl messages.mc
	mv messages.h ../include

EXTRA_DIST = \
	libnxsrv.dsp libnxsrv.dsw libnxsrv.rc \
	messages.mc messages.c \
	libnxsrv.h resource.h
