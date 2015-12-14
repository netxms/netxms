#!/bin/sh

HEADER="netxms-build-tag.h"

[ -f $HEADER ] || touch $HEADER

BUILD_TAG=`git describe 2>/dev/null`
[ -z $BUILD_TAG ] && BUILD_TAG=UNKNOWN
if ! grep -q "BUILDTAG:$BUILD_TAG" $HEADER; then
  echo "/* BUILDTAG:$BUILD_TAG */" > $HEADER
  echo "#ifndef _build_tag_h_" >> $HEADER
  echo "#define _build_tag_h_" >> $HEADER
  echo "#define NETXMS_BUILD_TAG _T(\"$BUILD_TAG\")" >> $HEADER
  echo "#endif" >> $HEADER
  echo "Build tag updated"
fi
