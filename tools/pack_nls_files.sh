#!/bin/sh

lang=$1
addon=$2

if [ "x$lang" = "x" ]; then
	echo "Usage: pack_nls_files.sh <lang>"
	exit 1
fi

cd src/java/netxms-eclipse

rm -f nxmc-messages-$lang.zip
find . -name messages_$lang.properties | grep -v bin | xargs zip -r nxmc-messages-$lang.zip
find . -name bundle_$lang.properties | grep -v bin | xargs zip -r nxmc-messages-$lang.zip
mv nxmc-messages-$lang.zip ../../..
cd ../../../

if [ "x$addon" != "x" ]; then
	cp nxmc-messages-$lang.zip $addon
	cd $addon
	find . -name messages_$lang.properties | grep -v bin | xargs zip -r nxmc-messages-$lang.zip
	find . -name bundle_$lang.properties | grep -v bin | xargs zip -r nxmc-messages-$lang.zip
	cd -
	mv $addon/nxmc-messages-$lang.zip .
fi
