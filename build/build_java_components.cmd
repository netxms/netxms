@ECHO OFF

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION^= ^| cut -d^= -f2') do @set VERSION=%%a
echo VERSION=%VERSION%

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION_BASE^= ^| cut -d^= -f2') do @set VERSION_BASE=%%a

cmd /C mvn -f src/pom.xml versions:set -DnewVersion=%VERSION% -DprocessAllModules=true
cmd /C mvn -f src/client/nxmc/java/pom.xml versions:set -DnewVersion=%VERSION%

cmd /C mvn -f src/pom.xml clean install -Dmaven.test.skip=true

cmd /C mvn -f src/client/nxmc/java/pom.xml -Dnetxms.build.disablePlatformProfile=true -Pweb clean package
copy /Y src\client\nxmc\java\target\nxmc-%VERSION%.war src\client\nxmc\java

cmd /C mvn -f src/client/nxmc/java/pom.xml -Pdesktop -Pstandalone clean package
copy /Y src\client\nxmc\java\target\nxmc-%VERSION%-standalone.jar src\client\nxmc\java

cmd /C mvn -f src/client/nxmc/java/pom.xml -Pdesktop clean package
copy /Y src\client\nxmc\java\target\nxmc-%VERSION%.jar src\client\nxmc\java

cmd /C mvn -f src/pom.xml versions:revert -DprocessAllModules=true
cmd /C mvn -f src/client/nxmc/java/pom.xml versions:revert

IF EXIST .\private\branding\build\build_java_components_hook.cmd CALL .\private\branding\build\build_java_components_hook.cmd
