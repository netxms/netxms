@echo off
set ARCH=x64
SET MSVC_VERSION=14.16.27023

rem *** Windows 7 SDK with Visual Studio 2017 Compiler
rem set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\bin\HostX64\x64;C:\SDK\Windows 7 SDK\Bin\x64;C:\SDK\Windows 7 SDK\Bin;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin;%PATH%
rem set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\ATLMFC\include;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\include;C:\Program Files (x86)\Windows Kits\10\include\10.0.10240.0\ucrt;C:\SDK\Windows 7 SDK\Include;C:\SDK\WINDDK\inc\wnet;%JAVA_HOME%\include;%JAVA_HOME%\include\win32;C:\SDK\VSSSDK72\inc;C:\SDK\libssh\include;C:\Program Files\Python37\include;C:\SDK\PCRE\x64\include
rem set LIB=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\ATLMFC\lib\x64;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\lib\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.10240.0\ucrt\x64;C:\SDK\Windows 7 SDK\Lib\x64;C:\SDK\WINDDK\lib\wnet\amd64;C:\SDK\libssh\lib\x64;C:\Program Files\Python37\libs;C:\SDK\MSVCRT\x64;;C:\SDK\PCRE\x64\lib
rem set LIBPATH=%LIB%

rem *** Windows 7 SDK with Visual Studio 2017 MSBuild
set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin;C:\SDK\Windows 7 SDK\Bin\x64;C:\SDK\Windows 7 SDK\Bin;%PATH%
