INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_aix.la
libnsm_aix_la_SOURCES = mac_dlpi.cpp main.cpp system.cpp
libnsm_aix_la_LDFLAGS = $(PTHREAD_LIBS) ../../../libnetxms/libnetxms.la

EXTRA_DIST = aix_subagent.h

install-exec-hook:
	$(AR) x $(libdir)/libnsm_aix.a $(libdir)/libnsm_aix.so.0
	(cd $(libdir) && rm -f libnsm_aix.so && $(LN_S) libnsm_aix.so.0 libnsm_aix.so)
