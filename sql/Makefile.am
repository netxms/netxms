#
# Makefile for SQL scripts
#

sqldir=$(pkgdatadir)/sql

sql_DATA = dbinit_mysql.sql \
           dbinit_pgsql.sql \
           dbinit_mssql.sql \
           dbinit_oracle.sql \
           dbinit_sqlite.sql \
           dbinit_db2.sql \
           dbschema_mysql.sql \
           dbschema_pgsql.sql \
           dbschema_mssql.sql \
           dbschema_oracle.sql \
           dbschema_sqlite.sql \
           dbschema_db2.sql

ROOT_FILE = dbinit.in
SOURCE = $(ROOT_FILE) database.in schema.in metadata.in setup.in events.in policy.in objtools.in scripts.in images.in traps.in
HEADER_FILES = ../include/netxmsdb.h ../include/nxevent.h ../src/server/include/nms_locks.h
COMMAND = $(CPP) @SQLFLAGS@ -P -D_NETXMS_DB_SCHEMA_ -I@top_srcdir@/include -I@top_srcdir@/src/server/include
SQUEEZE = perl -n strip

all: $(sql_DATA)

dbinit_mysql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_MYSQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_pgsql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_POSTGRESQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_mssql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_MSSQL $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_oracle.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_ORACLE $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_sqlite.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_SQLITE $(ROOT_FILE) | $(SQUEEZE) > $@

dbinit_db2.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_DB2 $(ROOT_FILE) | $(SQUEEZE) > $@

dbschema_mysql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_MYSQL -DSCHEMA_ONLY $(ROOT_FILE) | $(SQUEEZE) > $@

dbschema_pgsql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_POSTGRESQL -DSCHEMA_ONLY $(ROOT_FILE) | $(SQUEEZE) > $@

dbschema_mssql.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_MSSQL -DSCHEMA_ONLY $(ROOT_FILE) | $(SQUEEZE) > $@

dbschema_oracle.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_ORACLE -DSCHEMA_ONLY $(ROOT_FILE) | $(SQUEEZE) > $@

dbschema_sqlite.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_SQLITE -DSCHEMA_ONLY $(ROOT_FILE) | $(SQUEEZE) > $@

dbschema_db2.sql: $(SOURCE) $(HEADER_FILES)
	$(COMMAND) -DDB_DB2 -DSCHEMA_ONLY $(ROOT_FILE) | $(SQUEEZE) > $@

clean-local:
	rm -f dbinit_*.sql dbschema_*.sql

EXTRA_DIST = \
	$(sql_DATA) \
	$(SOURCE) \
	change-ansi-event_log.sql
