# Microsoft Developer Studio Project File - Name="libnetxms" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libnetxms - Win32 Debug UNICODE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libnetxms.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libnetxms.mak" CFG="libnetxms - Win32 Debug UNICODE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libnetxms - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnetxms - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnetxms - Win32 Debug AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnetxms - Win32 Release AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnetxms - Win32 Debug UNICODE" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnetxms - Win32 Release UNICODE" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libnetxms - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib dbghelp.lib crypt32.lib /nologo /version:0.1 /dll /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\libnetxms.dll C:\NetXMS\bin	copy Release\libnetxms.pdb C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnetxms - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib dbghelp.lib crypt32.lib /nologo /version:0.1 /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\libnetxms.dll ..\..\bin	copy Debug\libnetxms.pdb ..\..\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnetxms - Win32 Debug AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libnetxms___Win32_Debug_AMD64"
# PROP BASE Intermediate_Dir "libnetxms___Win32_Debug_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /Zi /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /D "__64BIT__" /D "_M_X64" /FD /Wp64 /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /version:0.1 /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib dbghelp.lib crypt32.lib /nologo /version:0.1 /dll /debug /machine:IX86 /pdbtype:sept /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug64\libnetxms.dll ..\..\bin64	copy Debug64\libnetxms.pdb ..\..\bin64
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnetxms - Win32 Release AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libnetxms___Win32_Release_AMD64"
# PROP BASE Intermediate_Dir "libnetxms___Win32_Release_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /D "__64BIT__" /D "_M_X64" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /version:0.1 /dll /machine:I386
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib dbghelp.lib crypt32.lib /nologo /version:0.1 /dll /debug /machine:IX86 /pdbtype:sept /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release64\libnetxms.dll C:\NetXMS\bin64	copy Release64\libnetxms.pdb C:\NetXMS\bin64
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnetxms - Win32 Debug UNICODE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libnetxms___Win32_Debug_UNICODE"
# PROP BASE Intermediate_Dir "libnetxms___Win32_Debug_UNICODE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_UNICODE"
# PROP Intermediate_Dir "Debug_UNICODE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\include" /D "UNICODE" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /version:0.1 /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib dbghelp.lib crypt32.lib /nologo /version:0.1 /dll /debug /machine:I386 /out:"Debug_UNICODE/libnetxmsw.dll" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug_UNICODE\libnetxmsw.dll ..\..\bin	copy Debug_UNICODE\libnetxmsw.pdb ..\..\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnetxms - Win32 Release UNICODE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libnetxms___Win32_Release_UNICODE"
# PROP BASE Intermediate_Dir "libnetxms___Win32_Release_UNICODE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_UNICODE"
# PROP Intermediate_Dir "Release_UNICODE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\include" /D "UNICODE" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNETXMS_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib ntdll.lib /nologo /version:0.1 /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libeay32.lib dbghelp.lib crypt32.lib /nologo /version:0.1 /dll /debug /machine:I386 /out:"Release_UNICODE/libnetxmsw.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release_UNICODE\libnetxmsw.dll C:\NetXMS\bin	copy Release_UNICODE\libnetxmsw.pdb C:\NetXMS\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libnetxms - Win32 Release"
# Name "libnetxms - Win32 Debug"
# Name "libnetxms - Win32 Debug AMD64"
# Name "libnetxms - Win32 Release AMD64"
# Name "libnetxms - Win32 Debug UNICODE"
# Name "libnetxms - Win32 Release UNICODE"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\config.cpp
# End Source File
# Begin Source File

SOURCE=.\crypto.cpp
# End Source File
# Begin Source File

SOURCE=.\dir.c
# End Source File
# Begin Source File

SOURCE=.\dload.cpp
# End Source File
# Begin Source File

SOURCE=.\gen_uuid.c
# End Source File
# Begin Source File

SOURCE=.\getopt.c
# End Source File
# Begin Source File

SOURCE=.\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\icmp.cpp
# End Source File
# Begin Source File

SOURCE=.\inline.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\md5.cpp
# End Source File
# Begin Source File

SOURCE=.\message.cpp
# End Source File
# Begin Source File

SOURCE=.\msgwq.cpp
# End Source File
# Begin Source File

SOURCE=.\nxcp.cpp
# End Source File
# Begin Source File

SOURCE=.\queue.cpp
# End Source File
# Begin Source File

SOURCE=.\regex.c
# End Source File
# Begin Source File

SOURCE=.\rwlock.cpp
# End Source File
# Begin Source File

SOURCE=.\scandir.c
# End Source File
# Begin Source File

SOURCE=.\seh.cpp
# End Source File
# Begin Source File

SOURCE=.\serial.cpp
# End Source File
# Begin Source File

SOURCE=.\sha1.cpp
# End Source File
# Begin Source File

SOURCE=.\StackWalker.cpp
# End Source File
# Begin Source File

SOURCE=.\string.cpp
# End Source File
# Begin Source File

SOURCE=.\strtoll.c
# End Source File
# Begin Source File

SOURCE=.\strtoull.c
# End Source File
# Begin Source File

SOURCE=.\table.cpp
# End Source File
# Begin Source File

SOURCE=.\tools.cpp
# End Source File
# Begin Source File

SOURCE=.\unicode.cpp
# End Source File
# Begin Source File

SOURCE=.\uuid.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\getopt.h
# End Source File
# Begin Source File

SOURCE=.\libnetxms.h
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE="..\..\include\netxms-regex.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\netxms-version.h"
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_agent.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_common.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_cscp.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_threads.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_util.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxcpapi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\include\rwlock.h
# End Source File
# Begin Source File

SOURCE=.\sha1.h
# End Source File
# Begin Source File

SOURCE=.\StackWalker.h
# End Source File
# Begin Source File

SOURCE=..\..\include\unicode.h
# End Source File
# Begin Source File

SOURCE=..\..\include\uuid.h
# End Source File
# Begin Source File

SOURCE=.\uuidP.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
