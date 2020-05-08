SOURCES = bridge.cpp dfile_info.cpp hwid.cpp lora_device_data.cpp main.cpp \
          procexec.cpp registry.cpp smbios.cpp tools.cpp ua_notification.cpp

lib_LTLIBRARIES = libnxagent.la

libnxagent_la_SOURCES = $(SOURCES)
libnxagent_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXAGENT_EXPORTS -I@top_srcdir@/build
libnxagent_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxagent_la_LIBADD = ../../db/libnxdb/libnxdb.la ../../libnetxms/libnetxms.la @LIBNXAGENT_LIBS@

EXTRA_DIST = libnxagent.h libnxagent.vcxproj libnxagent.vcxproj.filters wtsproc.cpp
