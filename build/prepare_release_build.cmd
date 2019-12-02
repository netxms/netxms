@ECHO OFF

SET POM_FILES=src/java-common/netxms-base/pom.xml;src/libnxjava/java/pom.xml;src/client/java/netxms-client/pom.xml;src/client/nxapisrv/java/pom.xml;src/client/nxshell/java/pom.xml;src/client/nxtcpproxy/pom.xml;src/agent/subagents/bind9/pom.xml;src/agent/subagents/java/java/pom.xml;src/agent/subagents/jmx/pom.xml;src/agent/subagents/ubntlw/pom.xml;src/mobile-agent/java;src/java/nxreporting/pom.xml

cd build
updatetag.pl
cd ..
copy build\netxms-build-tag.properties src\java-common\netxms-base\src\main\resources\ 

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION ^| cut -d^= -f2') do @set VERSION=%%a
echo VERSION=%VERSION%

for %%p in (%POM_FILES%) do mvn -f %%p versions:set -DnewVersion=%VERSION%
rem atlas-mvn -f src/java/netxms-jira-connector/pom.xml versions:set -DnewVersion=%VERSION%
