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

make=`which gmake`
[ "x"$make = "x" ] && make=make

case `uname -s` in
	Linux)
		pkill=killall
		;;
	SunOS)
		pkill=pkill
		;;
	*BSD)
		pkill=killall
		;;
	*)
		pkill=pkill
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

# ask nxagentd gently
$pkill nxagentd 2>/dev/null
# wait a few seconds and smash it down
sleep 15 && $pkill -9 nxagentd 2>/dev/null

# do configure
./configure --prefix=$prefix --with-agent $configureAdd 2>/dev/null >/dev/null
if [ $? != 0 ]; then
	echo configure failed, duh
   # Try to restart existing agent
   $prefix/bin/nxagentd -d >/dev/null 2>/dev/null
	exit 4
fi

# build
$make >/dev/null 2>/dev/null
if [ $? != 0 ]; then
	echo build failed, duh
   # Try to restart existing agent
   $prefix/bin/nxagentd -d >/dev/null 2>/dev/null
	exit 4
fi

# now we can install it...
$make install >/dev/null 2>/dev/null
if [ $? != 0 ]; then
	echo install failed
   # Try to restart existing agent
   $prefix/bin/nxagentd -d >/dev/null 2>/dev/null
	exit 5
fi

# and restart
$prefix/bin/nxagentd -d >/dev/null 2>/dev/null
if [ $? != 0 ]; then
	echo nxagentd not started
	exit 5
fi
