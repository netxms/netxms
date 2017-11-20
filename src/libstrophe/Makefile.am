lib_LTLIBRARIES = libstrophe.la
libstrophe_la_SOURCES = auth.c conn.c crypto.c ctx.c event.c handler.c hash.c jid.c \
                        md5.c parser_expat.c rand.c resolver.c sasl.c scram.c sha1.c \
                        sock.c stanza.c tls_openssl.c util.c uuid.c
libstrophe_la_CPPFLAGS = -I@top_srcdir@/include
libstrophe_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
libstrophe_la_LIBADD =
if USE_INTERNAL_EXPAT
libstrophe_la_CPPFLAGS += -I@top_srcdir@/src/libexpat/libexpat
libstrophe_la_LIBADD += ../libexpat/libexpat/libnxexpat.la
endif

EXTRA_DIST = \
	Makefile.w32 \
	GPL-LICENSE.txt LICENSE.txt MIT-LICENSE.txt \
	libstrophe.vcxproj libstrophe.vcxproj.filters \
	common.h \
	hash.h \
	md5.h \
	ostypes.h \
	parser.h \
	rand.h \
	resolver.h \
	sasl.h \
	scram.h \
	sha1.h \
	snprintf.h \
	sock.h \
	tls.h \
	util.h
