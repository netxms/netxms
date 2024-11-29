@ECHO OFF

cd build
perl updatetag.pl
cd ..
copy build\netxms-build-tag.properties src\java-common\netxms-base\src\main\resources\ 

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION^= ^| cut -d^= -f2') do @set VERSION=%%a
echo VERSION=%VERSION%

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION_BASE^= ^| cut -d^= -f2') do @set VERSION_BASE=%%a

type src\java\netxms-eclipse\Core\plugin.xml | sed -r "s,^(.*;Version) [0-9.]+(&#x0A.*)$,\1 %VERSION%\2," > plugin.xml
copy /Y plugin.xml src\java\netxms-eclipse\Core\plugin.xml
del plugin.xml

cd sql
make -f Makefile.w32
cd ..

echo chcp 65001 > build\update_exe_version.cmd
echo rcedit %%1 --set-file-version %VERSION_BASE% --set-product-version %VERSION_BASE% --set-version-string CompanyName "Raden Solutions" --set-version-string FileDescription %%2 --set-version-string ProductName "NetXMS" --set-version-string LegalCopyright "© 2023 Raden Solutions SIA. All Rights Reserved" >> build\update_exe_version.cmd

set NO_JAVA_BUILD=yes
IF EXIST .\private\branding\build\prepare_release_build_hook.cmd CALL .\private\branding\build\prepare_release_build_hook.cmd
