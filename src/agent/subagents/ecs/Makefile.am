INCLUDES=-I@top_srcdir@/include

SUBAGENT = ecs

pkglib_LTLIBRARIES = ecs.la
ecs_la_SOURCES = ecs.cpp
ecs_la_LDFLAGS = -module -avoid-version
ecs_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = ecs.vcproj

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(pkglibdir)/$(SUBAGENT).a $(pkglibdir)/$(SUBAGENT).so ; rm -f $(pkglibdir)/$(SUBAGENT).a ; fi
	mv $(pkglibdir)/$(SUBAGENT).so $(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(pkglibdir)/$(SUBAGENT).la
	rm -f $(libdir)/libnsm_$(SUBAGENT).so
	ln -s $(pkglibdir)/$(SUBAGENT).nsm $(libdir)/libnsm_$(SUBAGENT).so
