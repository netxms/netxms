SUBAGENT = java

pkglib_LTLIBRARIES = java.la
java_la_SOURCES = Action.cpp ConfigHelper.cpp ListParameter.cpp main.cpp \
                  Parameter.cpp ParameterType.cpp PushParameter.cpp \
                  SubAgent.cpp TableColumn.cpp TableParameter.cpp
java_la_CPPFLAGS=-I@top_srcdir@/include
java_la_LDFLAGS = -module -avoid-version
java_la_LIBADD = ../../../libnetxms/libnetxms.la

EXTRA_DIST = \
	java.vcproj \
	Action.h \
	ConfigHelper.h \
	java_subagent.h \
	JNIException.h \
	ListParameter.h \
	Parameter.h \
	ParameterType.h \
	PushParameter.h \
	SubAgent.h \
	TableColumn.h \
	TableParameter.h

SUBDIRS = java

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
