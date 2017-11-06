@echo off
set ARCH=x86

rem *** Windows 7 SDK with Visual Studio 2010 Compiler
rem set PATH=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin;C:\Program Files (x86)\Microsoft Visual Studio 10.0\Common7\IDE;C:\SDK\Windows 7 SDK\Bin;%PATH%
rem set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\include;C:\SDK\Windows 7 SDK\Include;C:\SDK\WINDDK\inc\wnet;%JAVA_HOME%\include;%JAVA_HOME%\include\win32
rem set LIB=C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\lib;C:\SDK\Windows 7 SDK\Lib;C:\SDK\WINDDK\lib\wnet\i386

rem *** Windows 7 SDK with Visual Studio 2017 Compiler
set PATH=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.11.25503\bin\HostX64\x86;C:\SDK\Windows 7 SDK\Bin;%PATH%
set INCLUDE=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.11.25503\ATLMFC\include;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.11.25503\include;C:\Program Files (x86)\Windows Kits\10\include\10.0.10240.0\ucrt;C:\SDK\Windows 7 SDK\Include;C:\SDK\WINDDK\inc\wnet;%JAVA_HOME%\include;%JAVA_HOME%\include\win32;C:\SDK\VSSSDK72\inc
set LIB=C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.11.25503\ATLMFC\lib\x86;C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.11.25503\lib\x86;C:\Program Files (x86)\Windows Kits\10\lib\10.0.10240.0\ucrt\x86;C:\SDK\Windows 7 SDK\Lib;C:\SDK\WINDDK\lib\wnet\i386
set LIBPATH=%LIB%
