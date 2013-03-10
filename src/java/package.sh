#!/bin/sh

mvn -Dmaven.test.skip=true package
mvn -Dmaven.test.skip=true install

version=`grep projectversion pom.xml | cut -f 2 -d \> | cut -f 1 -d \<`

rm -f \
  netxms-eclipse/core/jar/netxms-base*.jar \
  netxms-eclipse/core/jar/netxms-client*.jar \
  ../../android/src/console/libs/netxms-base*.jar \
  ../../android/src/console/libs/netxms-client*.jar \
  ../../webui/webapp/Core/jar/netxms-base*.jar \
  ../../webui/webapp/Core/jar/netxms-client*.jar

cp netxms-base/target/netxms-base-$version.jar netxms-eclipse/core/jar/
cp netxms-client-api/target/netxms-client-api-$version.jar netxms-eclipse/core/jar/
cp netxms-client/target/netxms-client-$version.jar netxms-eclipse/core/jar/

cp netxms-base/target/netxms-base-$version.jar ../../android/src/console/libs/
cp netxms-client-api/target/netxms-client-api-$version.jar ../../android/src/console/libs/
cp netxms-client/target/netxms-client-$version.jar ../../android/src/console/libs/

cp netxms-base/target/netxms-base-$version.jar ../../webui/webapp/Core/jar
cp netxms-client-api/target/netxms-client-api-$version.jar ../../webui/webapp/Core/jar
cp netxms-client/target/netxms-client-$version.jar ../../webui/webapp/Core/jar
