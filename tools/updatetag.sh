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

BUILD_TAG=`git describe --always 2>/dev/null`
[ $? -ne 0 ] && [ -f $HEADER ] && exit
[ -f $HEADER ] || touch $HEADER

[ -z $BUILD_TAG ] && BUILD_TAG=UNKNOWN
BUILD_TAG=`echo $BUILD_TAG | sed 's/^Release-//'`
VERSION_STRING=`echo $BUILD_TAG | sed 's/-g.*//' | sed 's/-/./;t;s/$/.0/'`
BUILD_NUMBER=`echo $VERSION_STRING | cut -d . -f 3`

grep "BUILDTAG:$BUILD_TAG" $HEADER >/dev/null 2>&1
if [ $? != 0 ]; then
  echo "/* BUILDTAG:$BUILD_TAG */" > $HEADER
  echo "#ifndef _${FILE_PREFIX}_build_tag_h_" >> $HEADER
  echo "#define _${FILE_PREFIX}_build_tag_h_" >> $HEADER
  echo "#define ${DEFINE_PREFIX}_BUILD_TAG _T(\"$BUILD_TAG\")" >> $HEADER
  echo "#define ${DEFINE_PREFIX}_BUILD_TAG_A \"$BUILD_TAG\"" >> $HEADER
  echo "#define ${DEFINE_PREFIX}_BUILD_NUMBER $BUILD_NUMBER" >> $HEADER
  echo "#define ${DEFINE_PREFIX}_VERSION_STRING _T(\"$VERSION_STRING\")" >> $HEADER
  echo "#define ${DEFINE_PREFIX}_VERSION_STRING_A \"$VERSION_STRING\"" >> $HEADER
  echo "#endif" >> $HEADER
  echo "Build tag updated"
fi
