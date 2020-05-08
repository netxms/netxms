bin_PROGRAMS = nxapush
nxapush_SOURCES = nxapush.cpp
nxapush_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxapush_LDFLAGS = @EXEC_LDFLAGS@
nxapush_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la @EXEC_LIBS@
if STATIC_BUILD
if ALL_STATIC
nxapush_LDFLAGS += -all-static
else
nxapush_LDFLAGS += -static
endif
endif

EXTRA_DIST = \
    nxapush.vcxproj nxapush.vcxproj.filters
