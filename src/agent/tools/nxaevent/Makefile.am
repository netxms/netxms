bin_PROGRAMS = nxaevent
nxaevent_SOURCES = nxaevent.cpp
nxaevent_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxaevent_LDFLAGS = @EXEC_LDFLAGS@
nxaevent_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
if STATIC_BUILD
if ALL_STATIC
nxaevent_LDFLAGS += -all-static
else
nxaevent_LDFLAGS += -static
endif
endif

EXTRA_DIST = \
    nxaevent.vcxproj nxaevent.vcxproj.filters
