INCLUDES=-I@top_srcdir@/include

SUBAGENT = odbcquery

pkglib_LTLIBRARIES = odbcquery.la
odbcquery_la_SOURCES = odbcquery.cpp odbcsapi.cpp
odbcquery_la_LDFLAGS = -module -avoid-version
odbcquery_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = odbcquery.vcproj odbcquery.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
