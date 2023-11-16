SOURCES = bridge.cpp crl.cpp dfile_info.cpp hwid.cpp \
	  main.cpp modbus.cpp optionlist.cpp procexec.cpp registry.cpp \
	  smbios.cpp tcpscan.cpp tftp.cpp tools.cpp ua_notification.cpp

lib_LTLIBRARIES = libnxagent.la

libnxagent_la_SOURCES = $(SOURCES)
libnxagent_la_CPPFLAGS=-I@top_srcdir@/include -DLIBNXAGENT_EXPORTS -I@top_srcdir@/build @LIBMODBUS_CPPFLAGS@
libnxagent_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION) @LIBMODBUS_LDFLAGS@
libnxagent_la_LIBADD = ../../db/libnxdb/libnxdb.la ../../libnetxms/libnetxms.la @LIBNXAGENT_LIBS@

EXTRA_DIST = libnxagent.h libnxagent.vcxproj libnxagent.vcxproj.filters wtsproc.cpp
