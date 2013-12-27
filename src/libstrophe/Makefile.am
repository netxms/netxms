lib_LTLIBRARIES = libstrophe.la
libstrophe_la_SOURCES = auth.c conn.c ctx.c event.c handler.c hash.c jid.c \
                        md5.c parser_expat.c sasl.c snprintf.c sock.c \
                        stanza.c thread.c tls_openssl.c util.c
libstrophe_la_CPPFLAGS=-I@top_srcdir@/include
libstrophe_la_LDFLAGS = -version-info $(NETXMS_LIBRARY_VERSION)
if USE_INTERNAL_EXPAT
libstrophe_la_CPPFLAGS+=-I@top_srcdir@/src/libexpat/libexpat
libstrophe_la_LIBADD = ../libexpat/libexpat/libnxexpat.la
endif

EXTRA_DIST = \
	GPL-LICENSE.txt LICENSE.txt MIT-LICENSE.txt \
	libstrophe.vcproj \
	common.h \
	hash.h \
	md5.h \
	ostypes.h \
	parser.h \
	sasl.h \
	sock.h \
	thread.h \
	tls.h \
	util.h
