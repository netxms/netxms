@echo off
set ARCH=x64
SET MSVC_VERSION=14.14.26428

rem *** Windows 7 SDK with Visual Studio 2010 Compiler
rem set PATH=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\amd64;C:\SDK\Windows 7 SDK\Bin\x64;C:\SDK\Windows 7 SDK\Bin;%PATH%
rem set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include;C:\SDK\Windows 7 SDK\Include;C:\SDK\WINDDK\inc\wnet;%JAVA_HOME%\include;%JAVA_HOME%\include\win32
rem set LIB=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib\amd64;C:\SDK\Windows 7 SDK\Lib\x64;C:\SDK\WINDDK\lib\wnet\amd64

rem *** Windows 7 SDK with Visual Studio 2017 Compiler
set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\bin\HostX64\x64;C:\SDK\Windows 7 SDK\Bin\x64;C:\SDK\Windows 7 SDK\Bin;%PATH%
set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\ATLMFC\include;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\include;C:\Program Files (x86)\Windows Kits\10\include\10.0.10240.0\ucrt;C:\SDK\Windows 7 SDK\Include;C:\SDK\WINDDK\inc\wnet;%JAVA_HOME%\include;%JAVA_HOME%\include\win32;C:\SDK\VSSSDK72\inc
set LIB=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\ATLMFC\lib\x64;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\%MSVC_VERSION%\lib\x64;C:\Program Files (x86)\Windows Kits\10\lib\10.0.10240.0\ucrt\x64;C:\SDK\Windows 7 SDK\Lib\x64;C:\SDK\WINDDK\lib\wnet\amd64

set LIBPATH=%LIB%
