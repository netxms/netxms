#
# Makefile for SQL scripts
#

ROOT_FILE = dbinit.in
SOURCE = $(ROOT_FILE) schema.in setup.in events.in
COMMAND = cpp -P -Wundef

all: dbinit_mysql.sql dbinit_pgsql.sql dbinit_mssql.sql

dbinit_mysql.sql: $(SOURCE)
	$(COMMAND) -DDB_MYSQL -o $@ $(ROOT_FILE)

dbinit_pgsql.sql: $(SOURCE)
	$(COMMAND) -DDB_POSTGRESQL -o $@ $(ROOT_FILE)

dbinit_mssql.sql: $(SOURCE)
	$(COMMAND) -DDB_MSSQL -o $@ $(ROOT_FILE)
