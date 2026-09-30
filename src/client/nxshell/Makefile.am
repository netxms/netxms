# Copyright (C) 2025 NetXMS Team <bugs@netxms.org>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

bin_PROGRAMS = nxshell
nxshell_SOURCES = client.cpp console.cpp navigation.cpp nxshell.cpp parser.cpp repl.cpp script.cpp session.cpp shell.cpp
nxshell_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
nxshell_LDFLAGS = @EXEC_LDFLAGS@
nxshell_LDADD = ../../libnetxms/libnetxms.la @EXEC_LIBS@ @NXSHELL_LIBS@
if USE_INTERNAL_JANSSON
nxshell_LDADD += @top_srcdir@/src/jansson/libnxjansson.la
else
nxshell_LDADD += -ljansson
endif

EXTRA_DIST = nxshell.h parser.h README.md

# nxai is an alternative name of nxshell that starts it as AI assistant client
install-exec-hook:
	cd $(DESTDIR)$(bindir) && rm -f nxai$(EXEEXT) && $(LN_S) nxshell$(EXEEXT) nxai$(EXEEXT)

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/nxai$(EXEEXT)
