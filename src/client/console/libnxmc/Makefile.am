INCLUDES=-I@top_srcdir@/include -I@top_srcdir@/src/client/console/include

lib_LTLIBRARIES = libnxmc.la
libnxmc_la_SOURCES = image.cpp main.cpp nxview.cpp view.cpp viewtracker.cpp
libnxmc_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libnxmc_la_LIBADD = ../../../libnetxms/libnetxms.la ../../../libnxcl/libnxcl.la

EXTRA_DIST = \
	libnxmc.dsp libnxmc.h nxview.h
