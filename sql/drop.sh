#!/bin/sh

P="psql netxms netxms"
list=`echo '\d'|$P|grep "| table |"|cut -d' ' -f4`
for f in $list; do
	sql="$sql drop table $f;"
done

echo $sql | $P
