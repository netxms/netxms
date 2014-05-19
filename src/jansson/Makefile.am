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
libnxjansson_la_CPPFLAGS=-I@top_srcdir@/include
libnxjansson_la_LDFLAGS = -no-undefined -export-symbols-regex '^json_' -version-info 10:0:6
libnxjansson_la_LIBADD = -lm

EXTRA_DIST = \
	hashtable.h \
	jansson.def \
	jansson.vcproj \
	jansson_private.h \
	lookup3.h \
	strbuffer.h \
	utf.h
