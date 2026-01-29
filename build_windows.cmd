@echo off
REM
REM NetXMS Windows Build Script
REM Convenience wrapper for building NetXMS on Windows with MinGW
REM

setlocal enabledelayedexpansion

echo.
echo ============================================
echo   NetXMS Windows Build
echo ============================================
echo.

REM Check for config.mingw
if not exist config.mingw (
    echo ERROR: config.mingw not found!
    echo.
    echo Please run setup_windows_build.cmd first, or:
    echo   copy config.mingw.template config.mingw
    echo   edit config.mingw
    echo.
    pause
    exit /b 1
)

REM Check for MinGW in PATH
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GCC not found in PATH
    echo.
    echo Please ensure MinGW is installed and in your PATH
    echo Example: set PATH=C:\msys64\mingw64\bin;%%PATH%%
    echo.
    pause
    exit /b 1
)

REM Check for Perl (needed for updatetag.pl)
where perl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: Perl not found in PATH
    echo Build tag generation will be skipped
    echo Install Strawberry Perl or ActivePerl for full functionality
    echo.
    set SKIP_TAG=1
) else (
    set SKIP_TAG=0
)

REM Check for make
where make >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: GNU Make not found in PATH
    echo.
    echo Please ensure MinGW's make is in your PATH
    echo.
    pause
    exit /b 1
)

REM Display configuration
echo Configuration:
gcc --version | findstr /C:"gcc"
echo.

REM Generate build tag
if %SKIP_TAG% EQU 0 (
    echo Generating build tag...
    perl tools\updatetag.pl
    if %ERRORLEVEL% NEQ 0 (
        echo WARNING: Build tag generation failed
        echo Continuing anyway...
    )
    echo.
)

REM Determine number of CPU cores for parallel build
if not defined NUMBER_OF_PROCESSORS (
    set JOBS=4
) else (
    set JOBS=%NUMBER_OF_PROCESSORS%
)

echo Starting build with %JOBS% parallel jobs...
echo.

REM Build
set START_TIME=%TIME%
make -f Makefile.w32 -j%JOBS%
set BUILD_RESULT=%ERRORLEVEL%
set END_TIME=%TIME%

echo.
if %BUILD_RESULT% EQU 0 (
    echo ============================================
    echo   Build completed successfully!
    echo ============================================
    echo.
    echo Binaries are in out\
    echo.
    echo To install, run:
    echo   make -f Makefile.w32 install
    echo.
) else (
    echo ============================================
    echo   Build FAILED!
    echo ============================================
    echo.
    echo Check the error messages above for details.
    echo.
    pause
    exit /b 1
)

endlocal
exit /b 0
