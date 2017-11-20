bin_PROGRAMS = nxapush
nxapush_SOURCES = nxapush.cpp
nxapush_CPPFLAGS=-I@top_srcdir@/include
nxapush_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la
if STATIC_BUILD
if ALL_STATIC
nxapush_LDFLAGS = -all-static
else
nxapush_LDFLAGS = -static
endif
endif

EXTRA_DIST = \
    Makefile.w32 \
    nxapush.vcxproj nxapush.vcxproj.filters
