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

./build/update_plugin_versions.py . $VERSION.0

cat src/java/netxms-eclipse/Core/plugin.xml | sed -E "s,^(.*;Version) [0-9.]+(&#x0A.*)\$,\\1 $VERSION.0\\2," > ./plugin.xml
mv ./plugin.xml src/java/netxms-eclipse/Core/plugin.xml

cat src/java/netxms-eclipse/Product/nxmc.product | sed -E "s,^Version [0-9.]+\$,Version $VERSION.0," > nxmc.product
mv ./nxmc.product src/java/netxms-eclipse/Product/nxmc.product

sed -i -E "s,org.netxms:netxms-base:[0-9.]+-SNAPSHOT,org.netxms:netxms-base:$VERSION-SNAPSHOT,g" android/agent/app/build.gradle
sed -i -E "s,org.netxms:netxms-mobile-agent:[0-9.]+-SNAPSHOT,org.netxms:netxms-mobile-agent:$VERSION-SNAPSHOT,g" android/agent/app/build.gradle
sed -i -E "s,org.netxms:netxms-client:[0-9.]+-SNAPSHOT,org.netxms:netxms-client:$VERSION-SNAPSHOT,g" android/console/app/build.gradle
sed -i -E "s,org.netxms:netxms-base:[0-9.]+-SNAPSHOT,org.netxms:netxms-base:$VERSION-SNAPSHOT,g" android/console/app/build.gradle

sed -i -E "s,NETXMS_VERSION=[0-9.]+-SNAPSHOT,NETXMS_VERSION=$VERSION-SNAPSHOT,g" src/java-common/netxms-base/src/main/resources/netxms-version.properties
