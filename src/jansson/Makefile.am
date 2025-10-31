LIBS = @JANSSON_LIBS@

lib_LTLIBRARIES = libnxjansson.la
libnxjansson_la_SOURCES = \
	dump.c \
	error.c \
	hashtable.c \
	hashtable_seed.c \
	load.c \
	memory.c \
	pack_unpack.c \
	strbuffer.c \
	strconv.c \
	utf.c \
	value.c
libnxjansson_la_CPPFLAGS=-I@top_srcdir@/include -DJANSSON_EXPORTS -I@top_srcdir@/build
libnxjansson_la_LDFLAGS = -no-undefined -export-symbols-regex '^json_' -version-info 10:0:6
libnxjansson_la_LIBADD = -lm

EXTRA_DIST = \
	hashtable.h \
	jansson.def \
	jansson.vcxproj jansson.vcxproj.filters \
	jansson_private.h \
	jansson_private_config.h \
	lookup3.h \
	strbuffer.h \
	utf.h
