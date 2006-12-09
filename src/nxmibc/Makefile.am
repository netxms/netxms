# Copyright (C) 2004, 2005 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

INCLUDES=-I@top_srcdir@/include

bin_PROGRAMS = nxmibc
nxmibc_SOURCES = array.cpp lex.mp.c mibparse.cpp nxmibc.cpp parser.tab.c
nxmibc_LDADD = ../libnetxms/libnetxms.la ../libnxsnmp/libnxsnmp.la

EXTRA_DIST = nxmibc.dsp nxmibc.dsw nxmibc.h mibparse.h parser.l parser.y parser.tab.h

lex.mp.c: parser.l parser.tab.c parser.tab.h
	$(LEX) -Pmp parser.l

parser.tab.c: parser.y
	$(YACC) -b parser -d -p mp -t -v parser.y
