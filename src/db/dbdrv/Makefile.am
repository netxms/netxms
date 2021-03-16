# Copyright (C) 2004 NetXMS Team <bugs@netxms.org>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

DRIVERS = @DB_DRIVERS@

SUBDIRS = $(DRIVERS)

EXTRA_LTLIBRARIES = db2.la informix.la mariadb.la mysql.la odbc.la oracle.la pgsql.la sqlite.la
pkglib_LTLIBRARIES = @DB_DRIVERS_LTLIBRARIES@

db2_la_SOURCES = db2/db2.cpp
db2_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @DB2_CPPFLAGS@
db2_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' @DB2_LDFLAGS@
db2_la_LIBADD = ../../libnetxms/libnetxms.la @DB2_LIBS@

informix_la_SOURCES = informix/informix.cpp
informix_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @INFORMIX_CPPFLAGS@
informix_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' @INFORMIX_LDFLAGS@
informix_la_LIBADD = ../../libnetxms/libnetxms.la @INFORMIX_LIBS@

mariadb_la_SOURCES = mariadb/mariadb.cpp
mariadb_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @MARIADB_CPPFLAGS@
mariadb_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' @MARIADB_LDFLAGS@
mariadb_la_LIBADD = ../../libnetxms/libnetxms.la @MARIADB_LIBS@

mysql_la_SOURCES = mysql/mysql.cpp
mysql_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @MYSQL_CPPFLAGS@
mysql_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' @MYSQL_LDFLAGS@
mysql_la_LIBADD = ../../libnetxms/libnetxms.la @MYSQL_LIBS@

odbc_la_SOURCES = odbc/odbc.cpp
odbc_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @ODBC_CPPFLAGS@
odbc_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' @ODBC_LDFLAGS@
odbc_la_LIBADD = ../../libnetxms/libnetxms.la @ODBC_LIBS@

oracle_la_SOURCES = oracle/oracle.cpp
oracle_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @ORACLE_CPPFLAGS@
oracle_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' @ORACLE_LDFLAGS@
oracle_la_LIBADD = ../../libnetxms/libnetxms.la @ORACLE_LIBS@

pgsql_la_SOURCES = pgsql/pgsql.cpp
pgsql_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build @PGSQL_CPPFLAGS@
pgsql_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' @PGSQL_LDFLAGS@
pgsql_la_LIBADD = ../../libnetxms/libnetxms.la @PGSQL_LIBS@

sqlite_la_SOURCES = sqlite/sqlite.cpp
sqlite_la_CPPFLAGS = -I@top_srcdir@/include -I@top_srcdir@/build
sqlite_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
sqlite_la_LIBADD = ../../libnetxms/libnetxms.la
if USE_INTERNAL_SQLITE
sqlite_la_CPPFLAGS += -I@top_srcdir@/src/sqlite
sqlite_la_LIBADD += ../../sqlite/libnxsqlite.la
else
sqlite_la_LIBADD += -lsqlite3
endif

install-exec-hook:
	mkdir -p $(DESTDIR)$(pkglibdir)/dbdrv ; for _DRIVER in $(DRIVERS); do if test "x`uname -s`" = "xAIX"; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$$_DRIVER.a $(DESTDIR)$(pkglibdir)/$$_DRIVER@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$$_DRIVER.a; fi ; mv -f $(DESTDIR)$(pkglibdir)/$$_DRIVER@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/dbdrv/$$_DRIVER.ddr ; rm -f $(DESTDIR)$(pkglibdir)/$$_DRIVER.la; done
