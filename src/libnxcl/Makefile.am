SOURCES = actions.cpp agentcfg.cpp alarms.cpp cert.cpp comm.cpp \
          datacoll.cpp epp.cpp eventdb.cpp events.cpp graph.cpp \
          layer2.cpp main.cpp mib.cpp \
          mp.cpp objects.cpp objtools.cpp package.cpp script.cpp \
          server.cpp session.cpp situation.cpp snmptrap.cpp \
	  snmp.cpp users.cpp

lib_LTLIBRARIES = libnxcl.la

libnxcl_la_SOURCES = $(SOURCES)
libnxcl_la_CPPFLAGS=-I@top_srcdir@/include
libnxcl_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxcl_la_LIBADD = ../libnetxms/libnetxms.la ../libnxmap/libnxmap.la

EXTRA_DIST = \
	libnxcl.vcproj \
	libnxcl.h
