INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnxcl.la
libnxcl_la_SOURCES = actions.cpp alarms.cpp comm.cpp datacoll.cpp epp.cpp eventdb.cpp events.cpp image.cpp main.cpp mib.cpp objects.cpp session.cpp snmptrap.cpp users.cpp
libnxcl_la_LDFLAGS = -version-info $(LIBNETXMS_LIBRARY_VERSION)

EXTRA_DIST = \
	libnxcl.dsp libnxcl.dsw \
	libnxcl.vcp libnxcl.vcw \
	libnxcl.h
