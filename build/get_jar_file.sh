#!/usr/bin/env bash
# vim: ts=3 sw=3 expandtab
# Parameters:
#   1) artifact id
#   2) artifact version
#   3) artifact group (optional, defaults to org.netxms)
#   4) additional build parameters (optional)

if [ -z "$2" ]; then
   echo Usage: $0 '<artifactId> <version> [groupId] [build parameters]'
   exit 1
fi

ARTIFACT="$1"
VERSION="$2"
GROUP="org.netxms"
if [ ! -z $3 ]; then
   GROUP="$3"
fi

FGROUP=`echo $GROUP | sed -e 's^\.^/^g'`
JAR_FILE="$HOME/.m2/repository/$FGROUP/$ARTIFACT/$VERSION/$ARTIFACT-$VERSION.jar"

if echo $VERSION | grep -q SNAPSHOT; then
   if [ -f "$JAR_FILE" ]; then
      cp "$JAR_FILE" . || exit 1
   else
      shift; shift; shift
      mvn -Dmaven.test.skip=true install $@
      mvn dependency:copy -Dartifact=$GROUP:$ARTIFACT:$VERSION -DoutputDirectory=. || exit 1
   fi
else
   if [ -f "$JAR_FILE" ]; then
      cp "$JAR_FILE" . || exit 1
   else
      mvn dependency:copy -Dartifact=$GROUP:$ARTIFACT:$VERSION -DoutputDirectory=. || exit 1
   fi
fi
