SUBAGENT = python

pylibdir = $(pkglibdir)
pylib_DATA = subagent.py

pkglib_LTLIBRARIES = python.la
python_la_SOURCES = bridge.cpp main.cpp plugin.cpp
python_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @PYTHON_CPPFLAGS@
python_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym @PYTHON_LDFLAGS@
python_la_LIBADD = @top_srcdir@/src/agent/libnxagent/libnxagent.la \
	@top_srcdir@/src/libnxpython/libnxpython.la \
	@top_srcdir@/src/libnetxms/libnetxms.la \
	@PYTHON_LIBS@

EXTRA_DIST = \
	python.vcxproj python.vcxproj.filters \
	python_subagent.h

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
