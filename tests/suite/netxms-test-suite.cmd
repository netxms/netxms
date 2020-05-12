@echo off

set BuildType=Release
if [%1]==[] (
	set BuildType=Release
) else (
	set BuildType=%1
)

echo *** Running NetXMS test suite *** && (
echo *** test-libnetxms ***
.\x64\%BuildType%\test-libnetxms.exe
) && (
echo *** test-libnxsnmp ***
.\x64\%BuildType%\test-libnxsnmp.exe
) && (
echo *** test-libnxsl ***
.\x64\%BuildType%\test-libnxsl.exe .\tests\test-libnxsl
) && (
echo *** SUCCESS ***
) || (
echo *** FAILURE ***
)
