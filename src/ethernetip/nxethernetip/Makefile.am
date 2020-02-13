bin_PROGRAMS = nxethernetip
nxethernetip_SOURCES = nxethernetip.cpp
nxethernetip_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build
nxethernetip_LDFLAGS = @EXEC_LDFLAGS@
nxethernetip_LDADD = ../libethernetip/libethernetip.la ../../libnetxms/libnetxms.la @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxethernetip.vcxproj nxethernetip.vcxproj.filters
