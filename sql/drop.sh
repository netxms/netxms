#!/bin/sh

for f in `grep "CREATE TABLE" dbinit_pgsql.sql|cut -d' ' -f3`; do echo "drop table $f;"; done #| psql netxms netxms
