@echo off

if "%1" == "" goto usage
set version=%1

"C:\Program Files\Debugging Tools for Windows (x64)\symstore" add /r /f C:\Source\...\Release\*.* /s \\internal.radensolutions.com\symbols /t "NetXMS" /v "Build %version%"

exit

:usage
echo Usage: uploadPDB.cmd VERSION
