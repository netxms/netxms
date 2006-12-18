INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_ecs.la
libnsm_ecs_la_SOURCES = ecs.cpp
libnsm_ecs_la_LDFLAGS = ../../../libnetxms/libnetxms.la

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_ecs.so.0 ; \
		$(AR) x $(libdir)/libnsm_ecs.a $(libdir)/libnsm_ecs.so.0 ; \
	fi
