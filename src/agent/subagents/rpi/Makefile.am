SUBAGENT = rpi

pkglib_LTLIBRARIES = rpi.la
rpi_la_SOURCES = htsensor.cpp rpi.cpp
rpi_la_CPPFLAGS=-I@top_srcdir@/include
rpi_la_LDFLAGS = -module -avoid-version
rpi_la_LIBADD = ../../../libnetxms/libnetxms.la -lbcm2835

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).so $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT).so
endif
