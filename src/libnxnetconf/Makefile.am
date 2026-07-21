SOURCES = capabilities.cpp framing.cpp main.cpp message.cpp response.cpp

lib_LTLIBRARIES = libnxnetconf.la

libnxnetconf_la_SOURCES = $(SOURCES)
libnxnetconf_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build -DLIBNXNETCONF_EXPORTS
libnxnetconf_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxnetconf_la_LIBADD = ../libnetxms/libnetxms.la

EXTRA_DIST = \
	Makefile.w32 \
	libnxnetconf.h
