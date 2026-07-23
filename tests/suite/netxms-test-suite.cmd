@echo off
rem
rem Run NetXMS test suite (Windows)
rem Copyright (c) 2020-2026 Raden Solutions
rem
rem Usage: netxms-test-suite.cmd [Release|Debug] [x64|x86|arm64]
rem
rem Binaries are taken from the output tree produced by the MinGW build
rem (out\<arch>\<build type>\bin). Run from the top of the source tree.
rem
rem The test binaries load third-party DLLs (OpenSSL, PCRE, zlib, expat, cURL,
rem libmodbus) from the build SDK. Set NETXMS_TEST_DLL_PATH to a semicolon-
rem separated list of directories holding them, or have them on PATH already.
rem "mingw32-make -f Makefile.w32 test" fills NETXMS_TEST_DLL_PATH in from the
rem SDK roots in config.mingw and runs this script.
rem

setlocal

set BuildType=%1
if "%BuildType%"=="" set BuildType=Release

set Arch=%2
if "%Arch%"=="" set Arch=x64

set BinDir=out\%Arch%\%BuildType%\bin

if not "%NETXMS_TEST_DLL_PATH%"=="" set PATH=%NETXMS_TEST_DLL_PATH%;%PATH%

if not exist "%BinDir%" (
	echo Build output directory %BinDir% not found
	exit /b 1
)

echo *** Running NetXMS test suite from %BinDir% ***

call :RunTest test-libnetxms || goto failure
call :RunTest test-libethernetip || goto failure
call :RunTest test-libnxsnmp || goto failure
call :RunTest test-libnxsl .\tests\test-libnxsl || goto failure
call :RunTest test-libnxsrv || goto failure
call :RunTest test-ncd-webhook || goto failure
call :RunTest test-unit-entsoe || goto failure
call :RunTest test-unit-openmeteo || goto failure

echo *** SUCCESS ***
exit /b 0

:failure
echo *** FAILURE ***
exit /b 1

rem
rem Run single test binary. Missing binaries are skipped, so a partial build
rem (agent only, no server, etc.) still runs the tests it does contain.
rem
:RunTest
if not exist "%BinDir%\%1.exe" (
	echo *** %1 - not built, skipped ***
	exit /b 0
)
echo.
echo *** %1 ***
"%BinDir%\%1.exe" %2
exit /b %ERRORLEVEL%
