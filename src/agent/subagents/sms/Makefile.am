INCLUDES=-I@top_srcdir@/include

lib_LTLIBRARIES = libnsm_sms.la
libnsm_sms_la_SOURCES = main.cpp sender.cpp
libnsm_sms_la_LDFLAGS = ../../../libnetxms/libnetxms.la

EXTRA_DIST = sms.h sms.dsw sms.vcproj

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then \
		rm -f $(libdir)/libnsm_sms.so.0 ; \
		$(AR) x $(libdir)/libnsm_sms.a $(libdir)/libnsm_sms.so.0 ; \
	fi
