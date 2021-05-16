#!/bin/sh

if [ "x$1" = "x" ]; then
	echo "Usage: $0 <version>"
	exit 1
fi

version=$1
major=`echo $version | cut -d . -f 1`
minor=`echo $version | cut -d . -f 2`
build=`echo $version | cut -d . -f 3`

git checkout "stable-$major.$minor"
current_build=`git describe --always | cut -d - -f 2`
build_diff=$(($current_build-$build))
git checkout HEAD~$build_diff
