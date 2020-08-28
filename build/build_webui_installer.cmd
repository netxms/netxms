@echo off
cmd /c mvn -f webui\webapp\pom.xml -Dtycho.mode=maven org.eclipse.tycho:tycho-versions-plugin:update-pom -Pweb
cmd /c mvn -f webui\webapp\pom.xml clean package -Pweb -Dtycho.disableP2Mirrors=true
copy webui\webapp\Product\target\nxmc.war src\install\windows\nxmc\
xcopy.exe /E /H /R /I /Y C:\SDK\ReleaseFiles files
cd src\install\windows
iscc /Ssigntool="C:\SDK\signtool.cmd $f" netxms-webui-x64.iss
cd ..\..\..\
