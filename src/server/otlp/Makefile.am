MODULE = otlp

BUILT_SOURCES = generated/common.pb.cc generated/resource.pb.cc generated/metrics.pb.cc generated/metrics_service.pb.cc

generated/%.pb.cc generated/%.pb.h: %.proto
	@mkdir -p generated
	@PROTOC@ --cpp_out=generated --proto_path=$(srcdir) $<

pkglib_LTLIBRARIES = otlp.la
otlp_la_SOURCES = main.cpp receiver.cpp matching.cpp instance.cpp
nodist_otlp_la_SOURCES = generated/common.pb.cc generated/resource.pb.cc generated/metrics.pb.cc generated/metrics_service.pb.cc
otlp_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/build \
                   @PROTOBUF_CPPFLAGS@ @MICROHTTPD_CPPFLAGS@ @ABSEIL_CPPFLAGS@
otlp_la_LDFLAGS = -module -avoid-version @PROTOBUF_LDFLAGS@ @ABSEIL_LDFLAGS@
otlp_la_LIBADD = ../../libnetxms/libnetxms.la ../libnxsrv/libnxsrv.la ../core/libnxcore.la \
                 @PROTOBUF_LIBS@ @MICROHTTPD_LIBS@

EXTRA_DIST = \
	otlp.h \
	common.proto resource.proto metrics.proto metrics_service.proto

CLEANFILES = generated/*.pb.cc generated/*.pb.h
clean-local:
	rm -rf generated

install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(MODULE).a $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(MODULE)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(MODULE).nxm
	rm -f $(DESTDIR)$(pkglibdir)/$(MODULE).la
