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
echo *** test-libethernetip ***
.\x64\%BuildType%\test-libethernetip.exe
) && (
echo *** test-libnxsnmp ***
.\x64\%BuildType%\test-libnxsnmp.exe
) && (
echo *** test-libnxsl ***
.\x64\%BuildType%\test-libnxsl.exe .\tests\test-libnxsl
) && (
echo *** test-libnxsrv ***
.\x64\%BuildType%\test-libnxsrv.exe
) && (
echo *** test-ncd-webhook ***
.\x64\%BuildType%\test-ncd-webhook.exe
) && (
echo *** test-unit-entsoe ***
.\x64\%BuildType%\test-unit-entsoe.exe
) && (
echo *** test-unit-openmeteo ***
.\x64\%BuildType%\test-unit-openmeteo.exe
) && (
echo *** SUCCESS ***
) || (
echo *** FAILURE ***
)
