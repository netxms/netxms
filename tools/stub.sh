#!/bin/sh

hash1=__HASH1__
hash2=__HASH2__
skip=__SKIP__
command=__COMMAND__
log="/tmp/agent_upgrade_log"
elog="/tmp/agent_upgrade_log.stderr"

trap '
	echo "Upgrade script finished" >> $log
	[ "x"$temp != "x" ] && rm -rf $temp
	exit
' INT EXIT

rm -f $log $elog

md5=`which md5 2>/dev/null`
if [ $? != 0 ]; then
	md5=`which md5sum 2>/dev/null`
	if [ $? != 0 ]; then
		md5=`which openssl 2>/dev/null`
		if [ $? != 0 ]; then
			echo "Can't calculate MD5, exiting" >> $log
			exit
		else
			md5="$md5 md5"
		fi
	fi
fi

if [ "X"`head -n$skip $0 | tail -n +5 | $md5 | cut -b1-32 | tr A-Z a-z` != "X"$hash1 ]; then
	echo "Script MD5 mismach" >> $log
	exit
fi

let "skip=skip+1" 2>/dev/null
if [ $? != 0 ]; then
	# real sh?
	skip=`let "$skip+1"`
fi

if [ "X"`tail -n +$skip $0 | $md5 | cut -b1-32 | tr A-Z a-z` != "X"$hash2 ]; then
	echo "Payload MD5 mismach" >> $log
	exit
fi

temp=`mktemp -d /tmp/nxinst.XXXXXX 2>/dev/null`
if [ $? != 0 ]; then
	echo "Can't create temp dir" >> $log
	exit 0
fi
tail -n +$skip $0 | gzip -dc 2>/dev/null | tar xf - -C $temp
if [ $? != 0 ]; then
	echo "Can't unpack" >> $log
	exit;
else
	cd $temp
	chmod +x ./$command
	if [ $? != 0 ]; then
		echo "Can't chmod $command" >> $log
		exit
	fi
	./$command >> $log 2> $elog
fi

exit
###################################
