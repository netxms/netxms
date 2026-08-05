# Copyright (C) 2025 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = nxai
nxai_SOURCES = client.cpp nxai.cpp repl.cpp session.cpp
nxai_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxai_LDFLAGS = @EXEC_LDFLAGS@
nxai_LDADD = ../../libnetxms/libnetxms.la @EXEC_LIBS@ @NXAI_LIBS@
if USE_INTERNAL_JANSSON
nxai_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxai_LDADD += -ljansson
endif

EXTRA_DIST = nxai.h README.md
