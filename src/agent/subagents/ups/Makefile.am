INCLUDES=-I@top_srcdir@/include

SUBAGENT = ups

pkglib_LTLIBRARIES = ups.la
ups_la_SOURCES = apc.cpp bcmxcp.cpp main.cpp microdowell.cpp serial.cpp ups.cpp 
ups_la_LDFLAGS = -module -avoid-version
ups_la_LIBADD= ../../../libnetxms/libnetxms.la

EXTRA_DIST = ups.h ups.vcproj usb.cpp

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
