#!/bin/sh

export MAVEN_OPTS='-Xmx512m -XX:MaxPermSize=128m'

set -e

cd api-build

mvn -N versions:update-child-modules
mvn clean
mvn -Dmaven.test.skip=true install

version=`grep '<version>' pom.xml | cut -f 2 -d \> | cut -f 1 -d \<|head -n1`

cd ..

rm -f \
  ../../../android/src/console/libs/netxms-base*.jar \
  ../../../android/src/console/libs/netxms-client*.jar \

cp netxms-base/target/netxms-base-$version.jar ../../../android/src/console/libs/
cp netxms-client/target/netxms-client-$version.jar ../../../android/src/console/libs/

