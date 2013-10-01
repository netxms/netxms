SUBAGENT = ups

pkglib_LTLIBRARIES = ups.la
ups_la_SOURCES = apc.cpp bcmxcp.cpp main.cpp microdowell.cpp serial.cpp ups.cpp 
ups_la_CPPFLAGS=-I@top_srcdir@/include
ups_la_LDFLAGS = -module -avoid-version
ups_la_LIBADD= ../../../libnetxms/libnetxms.la

EXTRA_DIST = ups.h ups.vcproj usb.cpp

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
