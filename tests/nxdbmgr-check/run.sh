#!/bin/bash
#
# Defect injection test for "nxdbmgr check".
#
# usage: run.sh [-d sqlite|pgsql|tsdb] [-H host] [-U user] [-b /path/to/nxdbmgr] [-k]
#
# -k keeps the work directory (logs, configuration, SQLite database) after a successful run.
#
# sqlite (default) uses a fresh database file in a scratch directory. pgsql and tsdb create a
# database named nxdbmgr_check_<random> on the given server with createdb (password from
# PGPASSWORD), and drop only that database on exit. The harness never reads an existing
# nxdbmgr configuration file.
#
# Steps:
#   1. init scratch database
#   2. baseline.sql, forced check (creates per-object data tables), then a check that must be clean
#   3. inject-defects.sql, forced check; log must contain every line of expected-first-run.txt and
#      none of unexpected-first-run.txt; assert-after-first-run.sql must pass
#   4. cleanup-report-only.sql, check must be clean; assert-after-cleanup.sql must pass
#   5. every batch that prints "SQL query failed" fails the run
#
# Assertion files are ordinary SQL batches: a false condition inserts a duplicate key into
# nxdbmgr_check_assert, which fails the statement (see baseline.sql).

set -u

DRIVER=sqlite
DBHOST=127.0.0.1
DBUSER=netxms
NXDBMGR=nxdbmgr
KEEP=0

while getopts "d:H:U:b:kh" opt; do
   case "$opt" in
      d) DRIVER="$OPTARG" ;;
      H) DBHOST="$OPTARG" ;;
      U) DBUSER="$OPTARG" ;;
      b) NXDBMGR="$OPTARG" ;;
      k) KEEP=1 ;;
      *) sed -n '2,24p' "$0"; exit 2 ;;
   esac
done

case "$DRIVER" in
   sqlite|pgsql|tsdb) ;;
   *) echo "Unsupported driver: $DRIVER" >&2; exit 2 ;;
esac

FIXTURES=$(cd "$(dirname "$0")" && pwd)
WORK=$(mktemp -d "${TMPDIR:-/tmp}/nxdbmgr-check.XXXXXX")
CONF="$WORK/nxdbmgr.conf"
CREATED_DB=""
STATUS=1

cleanup()
{
   if [ -n "$CREATED_DB" ]; then
      dropdb -h "$DBHOST" -U "$DBUSER" --if-exists "$CREATED_DB" || echo "WARNING: cannot drop database $CREATED_DB" >&2
   fi
   if [ "$STATUS" -eq 0 ] && [ "$KEEP" -eq 0 ]; then
      rm -rf "$WORK"
   else
      echo "Work directory kept: $WORK" >&2
   fi
}
trap cleanup EXIT

fail()
{
   echo "FAIL: $*" >&2
   exit 1
}

case "$DRIVER" in
   sqlite)
      printf 'DBDriver = sqlite\nDBName = %s/check.db\n' "$WORK" > "$CONF"
      ;;
   pgsql|tsdb)
      DBNAME="nxdbmgr_check_$(od -An -N4 -tx1 /dev/urandom | tr -d ' \n')"
      createdb -h "$DBHOST" -U "$DBUSER" "$DBNAME" || fail "cannot create database $DBNAME"
      CREATED_DB="$DBNAME"
      {
         printf 'DBDriver = pgsql\nDBServer = %s\nDBLogin = %s\nDBName = %s\n' "$DBHOST" "$DBUSER" "$DBNAME"
         [ -n "${PGPASSWORD:-}" ] && printf 'DBPassword = %s\n' "$PGPASSWORD"
      } > "$CONF"
      ;;
esac

# run_batch <sql file> <log name>
run_batch()
{
   [ -r "$1" ] || fail "cannot read batch file $1"
   "$NXDBMGR" -c "$CONF" -q batch "$1" > "$WORK/$2.log" 2>&1
   local rc=$?
   if grep -q 'SQL query failed\|Cannot load SQL command file' "$WORK/$2.log"; then
      cat "$WORK/$2.log" >&2
      fail "batch $1 failed"
   fi
   [ $rc -eq 0 ] || fail "batch $1 exited with $rc"
}

# run_check <log name> [nxdbmgr options...]
# Every check must exit 0 and run to completion; an aborted check is a harness failure even
# when the defect it hit would have been reported later.
run_check()
{
   local name="$1"
   shift
   "$NXDBMGR" -c "$CONF" -q "$@" check > "$WORK/$name.log" 2>&1
   local rc=$?
   [ $rc -eq 0 ] || fail "check '$name' exited with $rc (see $WORK/$name.log)"
   grep -q "Database check aborted" "$WORK/$name.log" && fail "check '$name' was aborted (see $WORK/$name.log)"
   grep -q "Database check completed" "$WORK/$name.log" || fail "check '$name' did not complete (see $WORK/$name.log)"
}

# assert_clean <log name>
assert_clean()
{
   grep -q "Database doesn't contain any errors" "$WORK/$1.log" || fail "check '$1' is not clean (see $WORK/$1.log)"
}

# assert_fragments <log name> <fragment file> present|absent
assert_fragments()
{
   [ -r "$2" ] || fail "cannot read fragment file $2"
   [ -r "$WORK/$1.log" ] || fail "cannot read log $WORK/$1.log"
   local failed=0
   while IFS= read -r line || [ -n "$line" ]; do
      case "$line" in
         ''|'#'*) continue ;;
      esac
      if grep -qF -- "$line" "$WORK/$1.log"; then
         [ "$3" = absent ] && { echo "unexpected in $1.log: $line" >&2; failed=1; }
      else
         [ "$3" = present ] && { echo "missing in $1.log: $line" >&2; failed=1; }
      fi
   done < "$2"
   [ $failed -eq 0 ] || fail "fragment check against $2"
}

echo "Driver: $DRIVER, work directory: $WORK"

echo "Step 1: init"
"$NXDBMGR" -c "$CONF" -q init "$DRIVER" > "$WORK/init.log" 2>&1 || { cat "$WORK/init.log" >&2; fail "init"; }

echo "Step 2: baseline"
run_batch "$FIXTURES/baseline.sql" baseline
run_check baseline-forced -f
run_check baseline-clean -E
assert_clean baseline-clean

echo "Step 3: inject defects"
run_batch "$FIXTURES/inject-defects.sql" inject
run_check first -f
assert_fragments first "$FIXTURES/expected-first-run.txt" present
assert_fragments first "$FIXTURES/unexpected-first-run.txt" absent
run_batch "$FIXTURES/assert-after-first-run.sql" assert-first

echo "Step 4: cleanup report-only defects"
run_batch "$FIXTURES/cleanup-report-only.sql" cleanup
run_check final -E
assert_clean final
run_batch "$FIXTURES/assert-after-cleanup.sql" assert-final

echo "PASS"
STATUS=0
