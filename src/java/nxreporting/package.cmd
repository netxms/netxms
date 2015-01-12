@echo off

del /q /s /f target\*

call mvn clean
call mvn -Dmaven.test.skip=true package %*

for /f "tokens=2 delims=>< " %%a in ('findstr "<version>" pom.xml') do (
  @set version=%%a
  goto :break
)
:break

cd target
zip -r ..\netxms-reporting-server-%version%.zip *.jar lib
cd ..
