#!/bin/sh

set -e

unset CDPATH cdpath

export MAVEN_OPTS='-Xmx512m'

cd netxms-base && mvn -T 1C clean install javadoc:jar && cd ..
cd netxms-client && mvn -T 1C clean install javadoc:jar && cd ..
cd mobile-agent && mvn -T 1C -Dmaven.test.skip=true clean install javadoc:jar && cd ..

version=`grep '<version>' netxms-base/pom.xml | cut -f 2 -d \> | cut -f 1 -d \<|head -n1`

rm -f \
  ../../../android/src/console/libs/netxms-base*.jar \
  ../../../android/src/console/libs/netxms-client*.jar \
