#
# Makefile for SQL scripts
#

sqldir=$(pkgdatadir)/sql

sql_SCRIPTS = dbinit_mysql.sql dbinit_pgsql.sql dbinit_mssql.sql

ROOT_FILE = dbinit.in
SOURCE = $(ROOT_FILE) schema.in setup.in events.in images.in
COMMAND = $(CPP) -x c -P -Wundef -D_NETXMS_DB_SCHEMA_ -I@top_srcdir@/include -I@top_srcdir@/src/server/include -I@top_srcdir@/src/server/core
SQUEEZE = perl -e "while (<>) { if (! /^[\s\t\n]*$$/) { print; } }"

all: $(sql_SCRIPTS)

dbinit_mysql.sql: $(SOURCE)
	$(COMMAND) -DDB_MYSQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_pgsql.sql: $(SOURCE)
	$(COMMAND) -DDB_POSTGRESQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_mssql.sql: $(SOURCE)
	$(COMMAND) -DDB_MSSQL $(ROOT_FILE) | $(SQUEEZE) > $@

clean:
	rm -f dbinit_*.sql
