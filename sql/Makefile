#
# Makefile for SQL scripts
#

ROOT_FILE = dbinit.in
SOURCE = $(ROOT_FILE) schema.in setup.in events.in
COMMAND = cpp -P -Wundef -I../include -I../src/server/core
SQUEEZE = perl -e "while (<>) { if (! /^[\s\t\n]*$$/) { print; } }"

all: dbinit_mysql.sql dbinit_pgsql.sql dbinit_mssql.sql

dbinit_mysql.sql: $(SOURCE)
	$(COMMAND) -DDB_MYSQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_pgsql.sql: $(SOURCE)
	$(COMMAND) -DDB_POSTGRESQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_mssql.sql: $(SOURCE)
	$(COMMAND) -DDB_MSSQL $(ROOT_FILE) | $(SQUEEZE) > $@

clean:
	rm -f dbinit_*.sql
