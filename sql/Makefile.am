#
# Makefile for SQL scripts
#

sqldir=$(pkgdatadir)/sql

sql_DATA = dbinit_mysql.sql dbinit_pgsql.sql dbinit_mssql.sql dbinit_oracle.sql

ROOT_FILE = dbinit.in
SOURCE = $(ROOT_FILE) schema.in setup.in events.in images.in policy.in
HEADER_FILES = ../include/netxmsdb.h ../include/nxevent.h ../include/nximage.h ../src/server/include/nms_locks.h
COMMAND = $(CPP) -x c -P -Wundef -D_NETXMS_DB_SCHEMA_ -I@top_srcdir@/include -I@top_srcdir@/src/server/include
SQUEEZE = perl -n -e "if (! /^[\s\t\n]*$$/) { print; }"

all: $(sql_DATA)

dbinit_mysql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_MYSQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_pgsql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_POSTGRESQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_mssql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_MSSQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_oracle.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_ORACLE $(ROOT_FILE) | $(SQUEEZE) > $@

clean:
	rm -f dbinit_*.sql

EXTRA_DIST = \
	$(sql_DATA) \
	dbinit.in events.in images.in \
	policy.in schema.in setup.in
