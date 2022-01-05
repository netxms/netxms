#!/bin/sh
exec='nxagentd'
if [ "x$1" != "x" ]; then
	exec="$1"
fi

if command -v gdb >/dev/null 2>&1; then
	DEBUGGER=gdb
else
	if command -v dbx >/dev/null 2>&1; then
		DEBUGGER=dbx
	else
		echo >&2 "gdb or dbx is required. Aborting."
		exit 1
	fi
fi

pid=`ps -ae | grep $exec | grep -v grep | grep -v capture | grep -v tail | grep -v less | grep -v log/$exec | grep -v $exec-asan | awk '{ print $1; }'`

if [ "x$pid" = "x" ]; then
    echo "Unable to find $exec process. Aborting."
    exit 1
fi

if [ `echo "$pid" | wc -l` -ne 1 ]; then
    echo "Found several $exec processes. Aborting."
    exit 1
fi

ts=`date +%Y%m%d-%H%M%S`
outputfile="/tmp/$exec-threads.$pid.$ts"
echo "Saving output to $outputfile"

cmdfile="/tmp/capture-$exec-threads.$DEBUGGER"

if [ "$DEBUGGER" = "gdb" ]; then
	echo "set height 0" > $cmdfile
	echo "set logging file $outputfile" >> $cmdfile
	echo "set logging on" >> $cmdfile
	echo "attach $pid" >> $cmdfile
	echo "thread apply all bt" >> $cmdfile
	echo "detach" >> $cmdfile
	echo "quit" >> $cmdfile
	gdb --batch-silent --command=$cmdfile "$exec"
fi

if [ "$DEBUGGER" = "dbx" ]; then
	echo "thread" > $cmdfile
	echo "detach" >> $cmdfile
	dbx -a $pid -c $cmdfile | awk '$1 ~ /\$t/ { gsub(/\$t/,"", $1); gsub(/\>/, "", $1); print $1 }' >$outputfile.ids 2>/dev/null

	rm -f $cmdfile
	for i in `cat $outputfile.ids`; do
		echo "print \"------------- tid $i -----------------\"" >> $cmdfile
		echo "thread current $i" >> $cmdfile
		echo "where" >> $cmdfile
		i=$(($i+1))
	done
	echo "detach" >> $cmdfile
	dbx -a $pid -c $cmdfile >$outputfile 2>/dev/null

	rm -f $outputfile.ids
fi

rm -f $cmdfile
