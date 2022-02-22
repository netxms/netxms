#!/bin/sh

set -e

cd build
./updatetag.pl
cd ..
cp build/netxms-build-tag.properties src/java-common/netxms-base/src/main/resources/

VERSION=`cat build/netxms-build-tag.properties | grep NETXMS_VERSION | cut -d = -f 2`

mvn -f src/pom.xml versions:set -DnewVersion=$VERSION -DprocessAllModules=true
mvn -f src/client/nxmc/java/pom.xml versions:set -DnewVersion=$VERSION

mvn -f src/pom.xml install -Dmaven.test.skip=true
mvn -f src/client/nxmc/java/pom.xml install -Pdesktop

mvn -f src/pom.xml versions:revert -DprocessAllModules=true
mvn -f src/client/nxmc/java/pom.xml versions:revert
