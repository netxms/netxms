INCLUDES=-I@top_srcdir@/include

SUBAGENT = sms

pkglib_LTLIBRARIES = sms.la
sms_la_SOURCES = main.cpp sender.cpp
sms_la_LDFLAGS = -module -avoid-version
sms_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = sms.h sms.vcproj

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
