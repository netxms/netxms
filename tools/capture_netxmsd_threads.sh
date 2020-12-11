#!/bin/sh
exec='netxmsd'
if [ "x$1" != "x" ]; then
	exec="$1"
fi

command -v gdb >/dev/null 2>&1 || { echo >&2 "gdb is required. Aborting."; exit 1; }

pid=`ps -ax | grep $exec | grep -v grep | grep -v capture_netxmsd_threads | grep -v tail | grep -v less | grep -v log/$exec | grep -v $exec-asan | awk '{ print $1; }'`

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

cmdfile="/tmp/capture_netxmsd_threads.gdb"
echo "set height 0" > $cmdfile
echo "set logging file $outputfile" >> $cmdfile
echo "set logging on" >> $cmdfile
echo "attach $pid" >> $cmdfile
echo "thread apply all bt" >> $cmdfile
echo "detach" >> $cmdfile
echo "quit" >> $cmdfile
gdb --batch-silent --command=$cmdfile "$exec"
rm $cmdfile
