SOURCES = attr.cpp epath.cpp helpers.cpp main.cpp message.cpp session.cpp

lib_LTLIBRARIES = libethernetip.la

libethernetip_la_SOURCES = $(SOURCES)
libethernetip_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build -DLIBETHERNETIP_EXPORTS
libethernetip_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libethernetip_la_LIBADD = ../../libnetxms/libnetxms.la

EXTRA_DIST = \
	libethernetip.vcxproj libethernetip.vcxproj.filters \
	libethernetip.h
