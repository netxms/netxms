#!/bin/sh

prefix=$1
version=$2
suffix=$3
if [ -z "$prefix" ]; then
	echo "Usage: tools/create_bin_apkg.sh <prefix> <version> [<suffix>]"
	exit 1
fi
if [ -z "$suffix" ]; then
	suffix=`uname -s`
fi

if [ ! -r $prefix/bin/nxagentd ]; then
	echo "Unable to find compiled agent undex $prefix"
	exit 1
fi

cd contrib
if [ $? != 0 ]; then
	echo "Cannot change directory to contrib. Do you run script from source tree root?"
	exit 1
fi

currdir=`pwd`
cd $prefix &&
tar cvf nxagent-binary.tar bin/ lib/ &&
gzip nxagent-binary.tar &&
cd $currdir &&
rm -rf inst &&
mkdir inst &&
mv $prefix/nxagent-binary.tar.gz inst/ &&
cp installBinaryAgent.sh inst/ &&
tar cvf inst.tar inst &&
gzip inst.tar &&
mv inst.tar.gz ../tools/ &&
cd ../tools/ &&
bash ./sharIt inst.tar.gz inst/installBinaryAgent.sh ../nxagent-$version-$suffix.apkg &&
cd .. &&
rm -rf contrib/inst
