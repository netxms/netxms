#!/bin/sh

FILE_PREFIX=$1
DEFINE_PREFIX=$2

if [ "x$FILE_PREFIX" = "x" ]; then
	FILE_PREFIX="netxms"
fi

if [ "x$DEFINE_PREFIX" = "x" ]; then
	DEFINE_PREFIX="NETXMS"
fi

HEADER="$FILE_PREFIX-build-tag.h"

[ -f $HEADER ] || touch $HEADER

BUILD_TAG=`git describe --always 2>/dev/null`
[ -z $BUILD_TAG ] && BUILD_TAG=UNKNOWN
grep "BUILDTAG:$BUILD_TAG" $HEADER >/dev/null 2>&1
if [ $? != 0 ]; then
  echo "/* BUILDTAG:$BUILD_TAG */" > $HEADER
  echo "#ifndef _${FILE_PREFIX}_build_tag_h_" >> $HEADER
  echo "#define _${FILE_PREFIX}_build_tag_h_" >> $HEADER
  echo "#define ${DEFINE_PREFIX}_BUILD_TAG _T(\"$BUILD_TAG\")" >> $HEADER
  echo "#define ${DEFINE_PREFIX}_BUILD_TAG_A \"$BUILD_TAG\"" >> $HEADER
  echo "#endif" >> $HEADER
  echo "Build tag updated"
fi
