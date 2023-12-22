#!/usr/bin/env bash
#
# Usage:
#    build_java_components.sh [-no-revert-version] [-skip-nxmc-build] [-build-all]
#

set -e

cd build
./updatetag.pl
cd ..
cp build/netxms-build-tag.properties src/java-common/netxms-base/src/main/resources/

VERSION=`cat build/netxms-build-tag.properties | grep NETXMS_VERSION= | cut -d = -f 2`

REVERT_VERSION="yes"
BUILD_NXMC="yes"
BUILD_ALL="no"

while [[ $# -gt 0 ]]; do
   case $1 in
      -build-all|all)
         BUILD_ALL="yes"
	 shift
	 ;;
      -no-revert-version)
         REVERT_VERSION="no"
	 shift
	 ;;
      -skip-nxmc-build)
         BUILD_NXMC="no"
	 shift
	 ;;
      *)
	 shift
	 ;;
   esac
done

if [ "$REVERT_VERSION" = "yes" ]; then
   trap '
      mvn -f src/pom.xml versions:revert -DprocessAllModules=true
      mvn -f src/client/nxmc/java/pom.xml versions:revert
   ' EXIT
fi

mvn -f src/pom.xml versions:set -DnewVersion=$VERSION -DprocessAllModules=true
mvn -f src/pom.xml clean install -Dmaven.test.skip=true

if [ "$BUILD_NXMC" = "yes" ]; then
   OS=`uname -s`
   if [ "$OS" = "Linux" -o "$OS" = "Darwin" ]; then
      mvn -f src/client/nxmc/java/pom.xml versions:set -DnewVersion=$VERSION

      if [ "$BUILD_ALL" = "yes" ]; then
         WORKIND_DIR='src/client/nxmc/java'
         mvn -f $WORKIND_DIR -Dmaven.test.skip=true -Pdesktop -Pstandalone clean package
         cp $WORKIND_DIR/target/nxmc-${VERSION}*-standalone.jar $WORKIND_DIR/
         mvn -f $WORKIND_DIR -Dmaven.test.skip=true -Dnetxms.build.disablePlatformProfile=true -Pweb clean package
         cp $WORKIND_DIR/target/nxmc-${VERSION}*.war $WORKIND_DIR/
         #build nxshell
         mvn -f src/client/nxshell/java -Dmaven.test.skip=true -Pstandalone clean package
      else
         mvn -f src/client/nxmc/java/pom.xml clean install -Pdesktop
      fi
   fi
fi
