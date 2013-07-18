#!/bin/sh

VERSION=$1
PREFIX=$2
if test "x$VERSION" = "x"; then
	echo "Usage: create_depot <version> <prefix>"
	exit
fi

if test "x$PREFIX" = "x"; then
	echo "Usage: create_depot <version> <prefix>"
	exit
fi

rm -rf ./build/bin ./build/lib ./build/etc ./build/share
cp -r $PREFIX/bin ./build/
cp -r $PREFIX/lib ./build/

mkdir build/init.d
cp ../../contrib/startup/hp-ux/nxagentd build/init.d/

mkdir build/etc
cp ../../contrib/nxagentd.conf-dist build/etc/nxagentd.conf

cat nxagent.psf | sed "s/@version@/$VERSION/" > nxagent.tmp.psf
/usr/sbin/swpackage -vv -s nxagent.tmp.psf -x media_type=tape -d ./nxagent-$VERSION-hpux-ia64.depot
gzip nxagent-$VERSION-hpux-ia64.depot
rm -f nxagent.tmp.psf
