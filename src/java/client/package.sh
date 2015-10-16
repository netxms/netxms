#!/bin/sh

export MAVEN_OPTS='-Xmx512m'

set -e

cd netxms-base && mvn clean install javadoc:jar && cd ..
cd netxms-client && mvn clean install javadoc:jar && cd ..
cd mobile-agent && mvn -Dmaven.test.skip=true clean install javadoc:jar && cd ..

version=`grep '<version>' netxms-base/pom.xml | cut -f 2 -d \> | cut -f 1 -d \<|head -n1`

rm -f \
  ../../../android/src/console/libs/netxms-base*.jar \
  ../../../android/src/console/libs/netxms-client*.jar \

cp netxms-base/target/netxms-base-$version.jar ../../../android/src/console/libs/
cp netxms-client/target/netxms-client-$version.jar ../../../android/src/console/libs/

cp netxms-base/target/netxms-base-$version.jar ../../../android/src/agent/libs/
cp mobile-agent/target/netxms-mobile-agent-$version.jar ../../../android/src/agent/libs/
