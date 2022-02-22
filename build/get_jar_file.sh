#!/bin/sh
# Parameters:
#   1) package name
#   2) package version
#   3) package group with / as separator (optional)
#   4) additional build parameters (optional)

GROUP="$3"
if [ "x$GROUP" = "x" ]; then
   GROUP="org/netxms"
fi
VERSION="$2"
JAR_FILE="$HOME/.m2/repository/$GROUP/$1/$VERSION/$1-$VERSION.jar"
SNAPSHOT=`echo $VERSION | grep SNAPSHOT | wc -l`
if [ ! -f "$JAR_FILE" -a $SNAPSHOT -eq 1 ]; then
   mvn -Dmaven.test.skip=true install $4
fi
cp "$JAR_FILE" .
