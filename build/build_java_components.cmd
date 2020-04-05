@ECHO OFF

call mvn -f src/java-common/netxms-base/pom.xml -Dmaven.test.skip=true -Ppkg-build install
call mvn -f src/client/java/netxms-client/pom.xml -Dmaven.test.skip=true -Ppkg-build install
call mvn -f src/client/nxshell/java/pom.xml -Dmaven.test.skip=true -Pnxshell-launcher package

SET POM_FILES=src/libnxjava/java/pom.xml;src/agent/subagents/java/java/pom.xml
for %%p in (%POM_FILES%) do mvn -f %%p -Dmaven.test.skip=true install

SET POM_FILES=src/client/nxapisrv/java/pom.xml;src/agent/subagents/bind9/pom.xml;src/agent/subagents/jmx/pom.xml;src/agent/subagents/ubntlw/pom.xml
for %%p in (%POM_FILES%) do mvn -f %%p -Dmaven.test.skip=true package
