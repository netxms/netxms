bin_PROGRAMS = nxhwid
nxhwid_SOURCES = nxhwid.cpp
nxhwid_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxhwid_LDFLAGS = @EXEC_LDFLAGS@
nxhwid_LDADD = @top_srcdir@/src/agent/libnxagent/libnxagent.la @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
if STATIC_BUILD
if ALL_STATIC
nxhwid_LDFLAGS += -all-static
else
nxhwid_LDFLAGS += -static
endif
endif

EXTRA_DIST = \
    nxhwid.vcxproj nxhwid.vcxproj.filters
