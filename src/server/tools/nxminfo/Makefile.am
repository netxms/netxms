bin_PROGRAMS = nxminfo
nxminfo_SOURCES = nxminfo.cpp
nxminfo_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include
nxminfo_LDFLAGS = @EXEC_LDFLAGS@
nxminfo_LDADD = ../../../libnetxms/libnetxms.la ../../libnxsrv/libnxsrv.la @SERVER_LIBS@ @EXEC_LIBS@

EXTRA_DIST = \
	Makefile.w32 \
	nxminfo.vcxproj nxminfo.vcxproj.filters
