#!/bin/sh

set -e

rm -rf target

mvn clean
mvn -Dmaven.test.skip=true package

version=`cat pom.xml | grep '<version>' | head -n 1 | cut -d '<' -f 2 | cut -d '>' -f 2`

cd target
cp -R ../sql .
zip -r ../netxms-reporting-server-$version.zip *.jar lib sql
cd ..
