#!/bin/sh
# Update base version in source tree

if [ ! -r configure.ac ]; then
	echo Must be run from root of source tree\!
	exit 1
fi

VERSION="$1"
if [ "x$VERSION" = "x" ]; then
	echo "Version must be passed as argument"
	exit 1
fi

set -e

mvn -f src/pom.xml versions:set -DnewVersion=$VERSION-SNAPSHOT -DprocessAllModules=true
mvn -f src/client/nxmc/java/pom.xml versions:set -DnewVersion=$VERSION-SNAPSHOT
mvn -f tests/integration/pom.xml versions:set -DnewVersion=$VERSION-SNAPSHOT

mvn -f src/pom.xml versions:commit -DnewVersion=$VERSION-SNAPSHOT -DprocessAllModules=true
mvn -f src/client/nxmc/java/pom.xml versions:commit -DnewVersion=$VERSION-SNAPSHOT
mvn -f tests/integration/pom.xml versions:commit -DnewVersion=$VERSION-SNAPSHOT

sed -i -E "s,NETXMS_VERSION=[0-9.]+-SNAPSHOT,NETXMS_VERSION=$VERSION-SNAPSHOT,g" src/java-common/netxms-base/src/main/resources/netxms-version.properties
