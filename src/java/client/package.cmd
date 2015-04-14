@echo off

del /q /s /f netxms-base\target\*
del /q /s /f netxms-client\target\*
del /q /s /f mobile-agent\target\*

call mvn -N versions:update-child-modules
call mvn clean
call mvn -Dmaven.test.skip=true install %*

for /f "tokens=2 delims=>< " %%a in ('findstr "<version>" pom.xml') do (
  @set version=%%a
  goto :break
)
:break

copy netxms-base\target\netxms-base-%version%.jar ..\..\..\android\src\console\libs\
copy netxms-client\target\netxms-client-%version%.jar ..\..\..\android\src\console\libs\

copy netxms-base\target\netxms-base-%version%.jar ..\..\..\android\src\agent\libs\
copy mobile-agent\target\netxms-mobile-agent-%version%.jar ..\..\..\android\src\agent\libs\
