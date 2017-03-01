#!/bin/sh

lang=$1
charset=$2

if [ "x$lang" = "x" -o "x$charset" = "x" ]; then
	echo "Usage: fixprop.sh <lang> <charset>"
	exit 1
fi

tool=`pwd`/fixprop

cd ../src/java

msgs=`find . -name messages_$lang.properties | grep -v bin | xargs echo`
bundles=`find . -name bundle_$lang.properties | grep -v bin | xargs echo`

for n in $msgs $bundles; do
	$tool $n $charset
done

cd - >/dev/null
