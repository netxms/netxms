INCLUDES=-I@top_srcdir@/include

SOURCES = actions.cpp agentcfg.cpp alarms.cpp cert.cpp comm.cpp \
          datacoll.cpp epp.cpp eventdb.cpp events.cpp graph.cpp \
          image.cpp layer2.cpp lpp.cpp main.cpp map.cpp mib.cpp \
          mp.cpp objects.cpp objtools.cpp package.cpp script.cpp \
          server.cpp session.cpp situation.cpp snmptrap.cpp \
	  snmp.cpp users.cpp

if BUILD_UNICODE_LIBS
lib_LTLIBRARIES = libnxcl.la libnxclw.la
else
lib_LTLIBRARIES = libnxcl.la
endif

libnxcl_la_SOURCES = $(SOURCES)
libnxcl_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxcl_la_LIBADD = ../libnetxms/libnetxms.la ../libnxmap/libnxmap.la

libnxclw_la_SOURCES = $(SOURCES)
libnxclw_la_CPPFLAGS = -DUNICODE
libnxclw_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxclw_la_LIBADD = ../libnetxms/libnetxmsw.la ../libnxmap/libnxmapw.la

EXTRA_DIST = \
	libnxcl.vcproj libnxclw.vcproj libnxcl.dsp \
	libnxcl.vcp libnxcl.vcw \
	libnxcl.h
