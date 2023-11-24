bin_PROGRAMS = nxmc
nxmc_SOURCES = nxmc.cpp
nxmc_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxmc_LDFLAGS = @EXEC_LDFLAGS@
nxmc_LDADD = @top_srcdir@/src/libnxjava/libnxjava.la @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@

EXTRA_DIST = nxmc.ico nxmc.rc nxmc.vcxproj nxmc.vcxproj.filters resource.h swt.manifest

if JAR_BUILD
SUBDIRS = java
else
SUBDIRS =
endif
