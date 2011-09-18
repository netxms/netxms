INCLUDES=-I@top_srcdir@/include

SUBAGENT = informix

pkglib_LTLIBRARIES = informix.la
informix_la_SOURCES = main.cpp
informix_la_LDFLAGS = -module -avoid-version
informix_la_LIBADD = ../../../libnetxms/libnetxms.la ../../../db/libnxdb/libnxdb.la

EXTRA_DIST = informix.vcproj informix_subagent.h

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
