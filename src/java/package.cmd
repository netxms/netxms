@echo off

call mvn -Dmaven.test.skip=true package %*
call mvn -Dmaven.test.skip=true install %*

for /f "tokens=2 delims=>< " %%a in ('findstr projectversion pom.xml') do @set version=%%a

cd W:\netxms\netxms\trunk\src\java\

copy netxms-base\target\netxms-base-%version%.jar netxms-eclipse\library\jar\
copy netxms-client\target\netxms-client-%version%.jar netxms-eclipse\library\jar\
