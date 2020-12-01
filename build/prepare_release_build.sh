#!/bin/sh
# Prepare source tree for release build

if [ ! -r configure.ac ]; then
	echo Must be run from root of source tree\!
	exit 1
fi

set -e

### Configuration
POM_FILES="src/java-common/netxms-base/pom.xml src/libnxjava/java/pom.xml src/client/java/netxms-client/pom.xml src/client/nxapisrv/java/pom.xml src/client/nxshell/java/pom.xml src/client/nxtcpproxy/pom.xml src/agent/subagents/bind9/pom.xml src/agent/subagents/java/java/pom.xml src/agent/subagents/jmx/pom.xml src/agent/subagents/ubntlw/pom.xml src/mobile-agent/java src/java/nxreporting/pom.xml test/integration/pom.xml"

### Code
cd build
./updatetag.pl
cd ..
cp build/netxms-build-tag.properties src/java-common/netxms-base/src/main/resources/ 

VERSION=`cat build/netxms-build-tag.properties | grep NETXMS_VERSION | cut -d = -f 2`

for pom in $POM_FILES; do
	mvn -f $pom versions:set -DnewVersion=$VERSION
done
atlas-mvn -f src/java/netxms-jira-connector/pom.xml versions:set -DnewVersion=$VERSION

./build/update_plugin_versions.py . $VERSION

cat src/java/netxms-eclipse/Core/plugin.xml | sed -E "s,^(.*;Version) [0-9.]+(&#x0A.*)\$,\\1 $VERSION\\2," > ./plugin.xml
mv ./plugin.xml src/java/netxms-eclipse/Core/plugin.xml

cat src/java/netxms-eclipse/Product/nxmc.product | sed -E "s,^Version [0-9.]+\$,Version $VERSION," > nxmc.product
mv ./nxmc.product src/java/netxms-eclipse/Product/nxmc.product

sed -i -e "s,org.netxms:netxms-client:[0-9].[0-9]-SNAPSHOT,org.netxms:netxms-client:$VERSION,g" android/agent/app/build.gradle
sed -i -e "s,org.netxms:netxms-mobile-agent:[0-9].[0-9]-SNAPSHOT,org.netxms:netxms-mobile-agent:$VERSION,g" android/agent/app/build.gradle
sed -i -e "s,org.netxms:netxms-client:[0-9].[0-9]-SNAPSHOT,org.netxms:netxms-client:$VERSION,g" android/console/app/build.gradle
sed -i -e "s,org.netxms:netxms-base:[0-9].[0-9]-SNAPSHOT,org.netxms:netxms-base:$VERSION,g" android/console/app/build.gradle

if [ -x ./private/branding/build/prepare_release_build_hook.sh ]; then
	./private/branding/build/prepare_release_build_hook.sh
fi

./reconf
