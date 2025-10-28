SUBAGENT = prometheus

BUILT_SOURCES = generated/remote.pb.cc generated/types.pb.cc

generated/%.pb.cc generated/%.pb.h: %.proto
	@mkdir -p generated
	@PROTOC@ --cpp_out=generated --proto_path=. $<

pkglib_LTLIBRARIES = prometheus.la
prometheus_la_SOURCES = main.cpp receiver.cpp handler.cpp printer.cpp
nodist_prometheus_la_SOURCES = generated/remote.pb.cc generated/types.pb.cc
prometheus_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build \
                         -I$(srcdir)/generated @PROTOBUF_CPPFLAGS@ @MICROHTTPD_CPPFLAGS@ @SNAPPY_CPPFLAGS@ @ABSEIL_CPPFLAGS@
prometheus_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
prometheus_la_LIBADD = ../../libnxagent/libnxagent.la \
                       ../../../libnetxms/libnetxms.la @PROTOBUF_LDFLAGS@ @PROTOBUF_LIBS@ @SNAPPY_LDFLAGS@ @SNAPPY_LIBS@ @MICROHTTPD_LDFLAGS@ @MICROHTTPD_LIBS@ @ABSEIL_LDFLAGS@
EXTRA_DIST = remote.proto types.proto prometheus.h

CLEANFILES = generated/*.pb.cc generated/*.pb.h
clean-local:
	rm -rf generated

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
