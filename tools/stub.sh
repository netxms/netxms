#!/bin/sh

hash1=__HASH1__
hash2=__HASH2__
skip=__SKIP__
skip1=__SKIP1__
command=__COMMAND__
command=inst/installAgent.sh
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

_PATH=$PATH
PATH=$PATH:`echo /opt/openssl*/bin | tr ' ' ':'`
export PATH
md5=
for app in openssl md5sum md5;
do
	tmp=`which $app 2>/dev/null`
	if [ $? = 0 ]; then
		echo $tmp | grep "no $app in " >/dev/null 2>&1
		if [ $?  != 0 ]; then
			md5=$tmp
			[ $app = "openssl" ] && md5="$md5 md5"
		fi
	fi
done
PATH=$_PATH
export PATH

tail="tail -n"
if [ `uname` = "SunOS" ]; then
	tail=tail
	mktemp() {
		d=/tmp/nxupdate.$$.UniQ
		mkdir $d && echo $d || false
	}
fi

if [ "x$md5" != "x" ]; then
	if [ "X"`head -n$skip $0 |
		$tail +5 |
		$md5 |
		cut -b1-32 |
		tr A-Z a-z` != "X"$hash1 ];
	then
		echo "Script MD5 mismach" >> $log
		exit
	fi

	if [ "X"`$tail +$skip1 $0 |
		$md5 |
		cut -b1-32 |
		tr A-Z a-z` != "X"$hash2 ];
	then
		echo "Payload MD5 mismach" >> $log
		exit
	fi
else
	echo "No md5/md5sum/openssl found, can't check integrity" >> $log
fi

temp=`mktemp -d /tmp/nxinst.XXXXXX 2>/dev/null`
if [ $? != 0 ]; then
	echo "Can't create temp dir" >> $log
	exit 0
fi
cd $temp
$tail +$skip1 $wd/$0 | gzip -dc 2>/dev/null | tar xf -
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
