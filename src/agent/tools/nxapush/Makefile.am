INCLUDES=-I@top_srcdir@/include

bin_PROGRAMS = nxapush
nxapush_SOURCES = nxapush.cpp
nxapush_LDADD = @top_srcdir@/src/libnetxms/libnetxms.la
if STATIC_BUILD
if ALL_STATIC
nxapush_LDFLAGS = -all-static
else
nxapush_LDFLAGS = -static
endif
endif

EXTRA_DIST = \
    nxapush.vcproj
