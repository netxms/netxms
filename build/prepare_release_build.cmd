@ECHO OFF

SET POM_FILES_INSTALL=src/java-common/netxms-base/pom.xml;src/libnxjava/java/pom.xml;src/client/java/netxms-client/pom.xml;src/agent/subagents/java/java/pom.xml;src/mobile-agent/java;src/server/nxreportd/java/pom.xml
SET POM_FILES_PACKAGE=src/agent/subagents/bind9/pom.xml;src/agent/subagents/jmx/pom.xml;src/agent/subagents/ubntlw/pom.xml;src/client/nxapisrv/java/pom.xml;src/client/nxshell/java/pom.xml;src/client/nxtcpproxy/pom.xml;tests/integration/pom.xml

cd build
updatetag.pl
cd ..
copy build\netxms-build-tag.properties src\java-common\netxms-base\src\main\resources\ 

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION ^| cut -d^= -f2') do @set VERSION=%%a
echo VERSION=%VERSION%

for %%p in (%POM_FILES_INSTALL%) do cmd /C mvn -f %%p -Dmaven.test.skip=true -Drevision=%VERSION% clean install
for %%p in (%POM_FILES_PACKAGE%) do cmd /C mvn -f %%p -Dmaven.test.skip=true -Drevision=%VERSION% clean package

cd sql
make -f Makefile.w32
cd ..
