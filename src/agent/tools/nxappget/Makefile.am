bin_PROGRAMS = nxappget
nxappget_SOURCES = nxappget.cpp
nxappget_CPPFLAGS=-I@top_srcdir@/include
nxappget_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/appagent/libappagent.la
if STATIC_BUILD
if ALL_STATIC
nxappget_LDFLAGS = -all-static
else
nxappget_LDFLAGS = -static
endif
endif

EXTRA_DIST = \
    Makefile.w32 \
    nxappget.vcxproj nxappget.vcxproj.filters
