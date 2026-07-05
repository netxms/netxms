# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

lib_LTLIBRARIES = libnxargon2.la
libnxargon2_la_SOURCES = \
	argon2.c \
	blake2/blake2b.c \
	core.c \
	encoding.c \
	ref.c \
	thread.c
libnxargon2_la_CPPFLAGS = -I@top_srcdir@/include/argon2 -I@top_srcdir@/src/libargon2 -I@top_srcdir@/build -DA2_VISCTL=1
libnxargon2_la_LDFLAGS = -no-undefined -export-symbols-regex '^argon2' -version-info 1:0:0

EXTRA_DIST = \
	LICENSE \
	blake2/blake2.h \
	blake2/blake2-impl.h \
	blake2/blamka-round-ref.h \
	core.h \
	encoding.h \
	thread.h
