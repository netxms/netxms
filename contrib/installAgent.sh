#!/bin/sh

#prefix=/opt/netxms
prefix=/usr/local
configureAdd=

##example #if [ `hostname --fqdn` = "host1.domain.tld" ]; then
#	prefix=/opt/nagios
#	configureAdd=--with-nice-option
#fi

###############################################################################
#
# Main code
#

case `uname -s` in
	Linux)
		pkill=killall
		make=make
		;;
	SunOS)
		pkill=pkill
		make=make
		;;
	*BSD)
		pkill=killall
		make=gmake
		;;
	*)
		pkill=pkill
		make=gmake
		;;
esac

cd `dirname $0`
name=`ls netxms-*.tar.gz 2>/dev/null|sed s',\.tar\.gz$,,'`
if [ "x$name" = "x" ]; then
	echo invalid package
	exit 1
fi

tar zxf $name.tar.gz 2>/dev/null
if [ $? != 0 ]; then
	echo invalid package
	exit 2
fi
cd $name
if [ $? != 0 ]; then
	echo invalid package
	exit 3
fi

# do configure
./configure --prefix=$prefix --with-agent $configureAdd 2>/dev/null >/dev/null
if [ $? != 0 ]; then
	echo configure failed, duh
	exit 4
fi

# build
$make >/dev/null 2>/dev/null
if [ $? != 0 ]; then
	echo build failed, duh
	exit 4
fi

# ask nxagentd gently
$pkill nxagentd 2>/dev/null
# wait a few seconds and smash it down
sleep 15 && $pkill -9 nxagentd 2>/dev/null

# now we can insall it...
$make install >/dev/null 2>/dev/null
if [ $? != 0 ]; then
	echo install failed
	exit 5
fi

# and restart
sleep 10
$prefix/bin/nxagentd -d >/dev/null 2>/dev/null
if [ $? != 0 ]; then
	echo nxagentd not started
	exit 5
fi
