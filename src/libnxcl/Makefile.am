INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxcl.la
libnxcl_la_SOURCES = actions.cpp agentcfg.cpp alarms.cpp cert.cpp comm.cpp \
		     datacoll.cpp epp.cpp eventdb.cpp events.cpp graph.cpp \
		     image.cpp layer2.cpp lpp.cpp main.cpp map.cpp mib.cpp \
		     mp.cpp objects.cpp objtools.cpp package.cpp script.cpp \
		     server.cpp session.cpp snmptrap.cpp snmp.cpp users.cpp
libnxcl_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxcl_la_LIBADD = ../libnetxms/libnetxms.la ../libnxmap/libnxmap.la

EXTRA_DIST = \
	libnxcl.dsp libnxcl.dsw \
	libnxcl.vcp libnxcl.vcw \
	libnxcl.h
