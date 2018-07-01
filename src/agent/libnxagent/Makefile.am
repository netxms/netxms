SOURCES = bridge.cpp dfile_info.cpp main.cpp procexec.cpp registry.cpp tools.cpp

lib_LTLIBRARIES = libnxagent.la

libnxagent_la_SOURCES = $(SOURCES)
libnxagent_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXAGENT_EXPORTS
libnxagent_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxagent_la_LIBADD = ../../db/libnxdb/libnxdb.la ../../libnetxms/libnetxms.la

EXTRA_DIST = Makefile.w32 libnxagent.h libnxagent.vcxproj libnxagent.vcxproj.filters
