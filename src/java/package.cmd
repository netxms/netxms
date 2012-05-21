@echo off

del /q /s /f netxms-base\target\*
del /q /s /f netxms-client-api\target\*
del /q /s /f netxms-client\target\*

call mvn -Dmaven.test.skip=true package %*
call mvn -Dmaven.test.skip=true install %*

for /f "tokens=2 delims=>< " %%a in ('findstr projectversion pom.xml') do @set version=%%a

rem cd W:\netxms\netxms\trunk\src\java\

copy netxms-base\target\netxms-base-%version%.jar netxms-eclipse\library\jar\
copy netxms-client-api\target\netxms-client-api-%version%.jar netxms-eclipse\library\jar\
copy netxms-client\target\netxms-client-%version%.jar netxms-eclipse\core\jar\

copy netxms-base\target\netxms-base-%version%.jar ..\..\android\src\console\libs\
copy netxms-client-api\target\netxms-client-api-%version%.jar ..\..\android\src\console\libs\
copy netxms-client\target\netxms-client-%version%.jar ..\..\android\src\console\libs\

copy netxms-base\target\netxms-base-%version%.jar ..\..\webui\webapp\Core\jar\
copy netxms-client-api\target\netxms-client-api-%version%.jar ..\..\webui\webapp\Core\jar\
copy netxms-client\target\netxms-client-%version%.jar ..\..\webui\webapp\Core\jar\
