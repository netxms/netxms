SUBAGENT = mongodb

pkglib_LTLIBRARIES = mongodb.la
mongodb_la_SOURCES = db.cpp main.cpp
mongodb_la_CPPFLAGS=-I@top_srcdir@/include @MONGODB_CPPFLAGS@
mongodb_la_LDFLAGS = -module -avoid-version @MONGODB_LDFLAGS@
mongodb_la_LIBADD = ../../../libnetxms/libnetxms.la @MONGODB_LIBS@

EXTRA_DIST = mongodb.vcproj mongodb_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
