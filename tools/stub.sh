#!/bin/sh

hash1=__HASH1__
hash2=__HASH2__
skip=__SKIP__
skip1=__SKIP1__
command=__COMMAND__
log=/tmp/nxagentupdate.log

wd=`pwd -P`

trap '
	echo "Upgrade script finished" >> $log
	cd $wd
	[ "x"$temp != "x" ] && rm -rf $temp
	exit
' INT EXIT

rm -f $log
echo "WD = $wd" >> $log

mktemp() {
	d=/tmp/nxupdate.$$.UniQ
	mkdir $d && echo $d || false
}

temp=`mktemp -d /tmp/nxinst.XXXXXX 2>/dev/null`
if [ $? != 0 ]; then
	echo "Can't create temp dir" >> $log
	exit 0
fi
cd $temp
tail +$skip1 $wd/$0 | gzip -dc 2>/dev/null | tar xf -
if [ $? != 0 ]; then
	echo "Can't unpack" >> $log
	exit;
else
	chmod +x ./$command
	if [ $? != 0 ]; then
		echo "Can't chmod $command" >> $log
		exit
	fi
	echo Starting $command $*... >> $log
	./$command $* 2>&1 >> $log
fi

exit
###################################
