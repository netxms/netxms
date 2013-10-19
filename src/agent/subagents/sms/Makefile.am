SUBAGENT = sms

pkglib_LTLIBRARIES = sms.la
sms_la_SOURCES = main.cpp sender.cpp
sms_la_CPPFLAGS=-I@top_srcdir@/include
sms_la_LDFLAGS = -module -avoid-version
sms_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = sms.h sms.vcproj

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
