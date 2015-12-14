#!/bin/sh

HEADER="netxms-build-tag.h"

git describe >/dev/null 2>/dev/null
if test $? -eq 0; then
	echo "Checking build tag"
	BUILD_TAG=`git describe`
	if test -f $HEADER; then
		found=`cat $HEADER | grep "BUILDTAG:$BUILD_TAG" | wc -l`
		if [ $found -gt 0 ]; then
			update="no"
		else
			update="yes"
		fi
	else
		update="yes"
	fi
	if test "x$update" = "xyes"; then
		echo "/* BUILDTAG:$BUILD_TAG */" > $HEADER
		echo "#ifndef _build_tag_h_" >> $HEADER
		echo "#define _build_tag_h_" >> $HEADER
		echo "#define NETXMS_BUILD_TAG _T(\"$BUILD_TAG\")" >> $HEADER
		echo "#endif" >> $HEADER
		echo "Build tag updated"
	fi
fi
