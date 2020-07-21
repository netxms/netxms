@ECHO OFF

SET SUBAGENT_POM_FILES=src/agent/subagents/bind9/pom.xml;src/agent/subagents/jmx/pom.xml;src/agent/subagents/ubntlw/pom.xml
SET POM_FILES=src/java-common/netxms-base/pom.xml;src/libnxjava/java/pom.xml;src/client/java/netxms-client/pom.xml;src/client/nxapisrv/java/pom.xml;src/client/nxshell/java/pom.xml;src/client/nxtcpproxy/pom.xml;src/agent/subagents/java/java/pom.xml;src/mobile-agent/java;src/java/nxreporting/pom.xml

cd build
updatetag.pl
cd ..
copy build\netxms-build-tag.properties src\java-common\netxms-base\src\main\resources\ 

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION ^| cut -d^= -f2') do @set VERSION=%%a
echo VERSION=%VERSION%

for %%p in (%POM_FILES%) do cmd /C mvn -f %%p versions:set -DnewVersion=%VERSION%
for %%p in (%SUBAGENT_POM_FILES%) do cmd /C mvn -f %%p versions:set -DnewVersion=%VERSION%

cmd /C mvn -f src/java-common/netxms-base/pom.xml -Dmaven.test.skip=true install -Ppkg-build
cmd /C mvn -f src/client/java/netxms-client/pom.xml -Dmaven.test.skip=true install -Ppkg-build
cmd /C mvn -f src/libnxjava/java/pom.xml -Dmaven.test.skip=true install
cmd /C mvn -f src/agent/subagents/java/java/pom.xml -Dmaven.test.skip=true install
cmd /C mvn -f src/client/nxshell/java/pom.xml package -Pnxshell-launcher
for %%p in (%SUBAGENT_POM_FILES%) do cmd /C mvn -f %%p package

cd sql
make -f Makefile.w32
cd ..
