@echo off
REM
REM NetXMS Windows Build Setup Script
REM First-time setup for building NetXMS on Windows with MinGW
REM

setlocal enabledelayedexpansion

echo.
echo ============================================
echo   NetXMS Windows Build Setup
echo ============================================
echo.

REM Check if config.mingw already exists
if exist config.mingw (
    echo config.mingw already exists.
    echo.
    choice /C YN /M "Overwrite with template"
    if errorlevel 2 goto :skip_config
    echo.
)

REM Copy template
echo Creating config.mingw from template...
copy /Y config.mingw.template config.mingw >nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Failed to create config.mingw
    pause
    exit /b 1
)

echo config.mingw created successfully.
echo.
echo ============================================
echo   IMPORTANT: Edit config.mingw
echo ============================================
echo.
echo You must edit config.mingw and set the SDK paths
echo to match your environment before building.
echo.
echo Key settings to review:
echo   - ARCH (x86, x64, or arm64)
echo   - OPENSSL_ROOT
echo   - PCRE_ROOT
echo   - PGSQL_ROOT, MYSQL_ROOT, etc.
echo   - WITH_* options for optional features
echo.

choice /C YN /M "Open config.mingw in notepad now"
if not errorlevel 2 (
    notepad config.mingw
)

:skip_config

echo.
echo ============================================
echo   Prerequisites Check
echo ============================================
echo.

REM Check for MinGW
where gcc >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] GCC found:
    gcc --version | findstr /C:"gcc"
) else (
    echo [MISSING] GCC not found in PATH
    echo.
    echo You need to install MinGW-w64.
    echo Recommended: Install MSYS2 from https://www.msys2.org/
    echo Then run: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
    echo.
)

REM Check for make
where make >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] GNU Make found
) else (
    echo [MISSING] GNU Make not found in PATH
    echo Install via MSYS2: pacman -S make
    echo.
)

REM Check for perl
where perl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] Perl found
) else (
    echo [MISSING] Perl not found in PATH
    echo Install Strawberry Perl from https://strawberryperl.com/
    echo or ActivePerl from https://www.activestate.com/products/perl/
    echo.
)

REM Check for windres
where windres >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [OK] windres found
) else (
    echo [MISSING] windres not found in PATH
    echo This is part of MinGW binutils
    echo.
)

echo.
echo ============================================
echo   Next Steps
echo ============================================
echo.
echo 1. Ensure all prerequisites are installed
echo 2. Edit config.mingw with your SDK paths
echo 3. Run: build_windows.cmd
echo.
echo For detailed instructions, see BUILD_WINDOWS.md
echo.

pause
endlocal
exit /b 0
