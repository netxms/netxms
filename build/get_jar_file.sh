#!/bin/sh
# vim: ts=3 sw=3 expandtab
# Parameters:
#   1) artifact id
#   2) artifact version
#   3) artifact group (optional, defaults to org.netxms)
#   4) additional build parameters (optional)

if [ -z $2 ]; then
   echo Usage: $0 '<artifactId> <version> [groupId] [builf parameters]'
   exit 1
fi

ARTIFACT="$1"
VERSION="$2"
GROUP="org.netxms"
if [ ! -z $3 ]; then
   GROUP="$3"
fi

mvn dependency:copy -Dartifact=$GROUP:$ARTIFACT:$VERSION -DoutputDirectory=. && exit 0

if echo $VERSION | grep -q SNAPSHOT; then
   shift; shift; shift
   mvn -Dmaven.test.skip=true install $@
   mvn dependency:copy -Dartifact=$GROUP:$ARTIFACT:$VERSION -DoutputDirectory=.
fi
