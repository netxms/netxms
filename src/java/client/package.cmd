@echo off

del /q /s /f netxms-base\target\*
del /q /s /f netxms-client\target\*
del /q /s /f mobile-agent\target\*

cd netxms-base
mvn clean install javadoc:jar
cd ..

cd netxms-client
mvn clean install javadoc:jar
cd ..

cd mobile-agent
mvn -Dmaven.test.skip=true clean install javadoc:jar
cd ..

for /f "tokens=2 delims=>< " %%a in ('findstr "<version>" netxms-base\pom.xml') do (
  @set version=%%a
  goto :break
)
:break

copy netxms-base\target\netxms-base-%version%.jar ..\..\..\android\src\console\libs\
copy netxms-client\target\netxms-client-%version%.jar ..\..\..\android\src\console\libs\

copy netxms-base\target\netxms-base-%version%.jar ..\..\..\android\src\agent\libs\
copy mobile-agent\target\netxms-mobile-agent-%version%.jar ..\..\..\android\src\agent\libs\
