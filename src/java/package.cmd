@echo off

del /q /s /f netxms-base\target\*
del /q /s /f netxms-client-api\target\*
del /q /s /f netxms-client\target\*
del /q /s /f mobile-agent\target\*
del /q /s /f certificate-manager\target\*

call mvn -N versions:update-child-modules
call mvn clean
call mvn -Dmaven.test.skip=true package %*
call mvn -Dmaven.test.skip=true install %*

for /f "tokens=2 delims=>< " %%a in ('findstr "<version>" pom.xml') do @set version=%%a

copy netxms-base\target\netxms-base-%version%.jar netxms-eclipse\core\jar\
copy netxms-client-api\target\netxms-client-api-%version%.jar netxms-eclipse\core\jar\
copy netxms-client\target\netxms-client-%version%.jar netxms-eclipse\core\jar\
copy certificate-manager\target\certificate-manager-%version%.jar netxms-eclipse\core\jar\

copy netxms-base\target\netxms-base-%version%.jar ..\..\webui\webapp\Core\jar\
copy netxms-client-api\target\netxms-client-api-%version%.jar ..\..\webui\webapp\Core\jar\
copy netxms-client\target\netxms-client-%version%.jar ..\..\webui\webapp\Core\jar\
copy certificate-manager\target\certificate-manager-%version%.jar ..\..\webui\webapp\Core\jar\

copy netxms-base\target\netxms-base-%version%.jar ..\..\android\src\console\libs\
copy netxms-client-api\target\netxms-client-api-%version%.jar ..\..\android\src\console\libs\
copy netxms-client\target\netxms-client-%version%.jar ..\..\android\src\console\libs\
copy certificate-manager\target\certificate-manager-%version%.jar ..\..\android\src\console\libs\

copy netxms-base\target\netxms-base-%version%.jar ..\..\android\src\agent\libs\
copy mobile-agent\target\netxms-mobile-agent-%version%.jar ..\..\android\src\agent\libs\
