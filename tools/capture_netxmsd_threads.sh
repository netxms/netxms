#!/bin/sh
exec='netxmsd'
if [ "x$1" != "x" ]; then
	exec="$1"
fi
pid=`ps -ax | grep netxmsd | grep -v grep | grep -v capture_netxmsd_threads | awk '{ print $1; }'`
ts=`date +%Y%m%d-%H%M%S`
cmdfile="/tmp/capture_netxmsd_threads.gdb"
echo "set height 0" > $cmdfile
echo "set logging file /tmp/netxmsd-threads.$pid.$ts" >> $cmdfile
echo "set logging on" >> $cmdfile
echo "attach $pid" >> $cmdfile
echo "thread apply all bt" >> $cmdfile
echo "detach" >> $cmdfile
echo "quit" >> $cmdfile
gdb --batch-silent --command=$cmdfile "$exec"
rm $cmdfile
