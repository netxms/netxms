INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_skeleton.la
libnsm_skeleton_la_SOURCES = skeleton.cpp
libnsm_skeleton_la_LDFLAGS = ../../../libnetxms/libnetxms.la

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_skeleton.so.0 ; \
		$(AR) x $(libdir)/libnsm_skeleton.a $(libdir)/libnsm_skeleton.so.0 ; \
	fi
