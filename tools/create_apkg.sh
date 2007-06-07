#!/bin/sh

version=$1
if [ -z "$version" ]; then
	echo "Usage: tools/create_apkg.sh <version>"
	exit 1
fi

if [ ! -r netxms-$version.tar.gz ]; then
	echo "Unable to find distribution package netxms-$version.tar.gz"
	exit 1
fi

cd contrib
if [ $? != 0 ]; then
	echo "Cannot change directory to contrib. Do you run script from source tree root?"
	exit 1
fi

rm -rf inst &&
mkdir inst &&
cp ../netxms-$version.tar.gz inst/ &&
cp installAgent.sh inst/ &&
tar zcvf inst.tar.gz inst &&
mv inst.tar.gz ../tools/ &&
cd ../tools/ &&
bash ./sharIt inst.tar.gz inst/installAgent.sh ../nxagent-$version.apkg &&
cd .. &&
rm -rf contrib/inst
