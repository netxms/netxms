bin_PROGRAMS = netxmsd
netxmsd_SOURCES = netxmsd.cpp
netxmsd_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/src/server/include @PYTHON_CPPFLAGS@
netxmsd_LDFLAGS = @PYTHON_LDFLAGS@
netxmsd_LDADD = \
	../core/libnxcore.la \
	../libnxsrv/libnxsrv.la \
	../../snmp/libnxsnmp/libnxsnmp.la \
	../../libnxsl/libnxsl.la \
	../../libnxlp/libnxlp.la \
	../../db/libnxdb/libnxdb.la \
	../../agent/libnxagent/libnxagent.la \
	../../libnxjava/libnxjava.la \
	../../libnetxms/libnetxms.la \
   @SERVER_LIBS@

if PYTHON_SUPPORT
netxmsd_LDADD += ../../libnxpython/libnxpython.la @PYTHON_LIBS@
endif

EXTRA_DIST = \
	netxmsd.vcxproj netxmsd.vcxproj.filters \
	netxmsd.h \
	winsrv.cpp \
	Makefile.w32
