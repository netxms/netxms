INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxcl.la
libnxcl_la_SOURCES = actions.cpp agentcfg.cpp alarms.cpp comm.cpp datacoll.cpp \
		     epp.cpp eventdb.cpp events.cpp image.cpp lpp.cpp main.cpp \
		     map.cpp mib.cpp objects.cpp objtools.cpp package.cpp \
		     script.cpp server.cpp session.cpp snmptrap.cpp snmp.cpp \
		     users.cpp
libnxcl_la_LDFLAGS = -version-info $(LIBNXCL_LIBRARY_VERSION)

EXTRA_DIST = \
	libnxcl.dsp libnxcl.dsw \
	libnxcl.vcp libnxcl.vcw \
	libnxcl.h
