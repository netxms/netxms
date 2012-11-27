INCLUDES=-I@top_srcdir@/include

bin_PROGRAMS = nxappget
nxappget_SOURCES = nxappget.cpp
nxappget_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @top_srcdir@/src/agent/appagent/libappagent.la
if STATIC_BUILD
if ALL_STATIC
nxappget_LDFLAGS = -all-static
else
nxappget_LDFLAGS = -static
endif
endif

EXTRA_DIST = \
    nxappget.vcproj
