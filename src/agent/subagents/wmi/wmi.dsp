# Microsoft Developer Studio Project File - Name="wmi" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=wmi - Win32 Debug AMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wmi.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wmi.mak" CFG="wmi - Win32 Debug AMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wmi - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wmi - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wmi - Win32 Release AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "wmi - Win32 Debug AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wmi - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib wbemuuid.lib /nologo /dll /machine:I386 /out:"Release/wmi.nsm" /pdbtype:sept /libpath:"..\..\..\libnetxms\Release"
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\wmi.nsm C:\NetXMS\bin	copy Release\wmi.pdb C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "wmi - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib wbemuuid.lib /nologo /dll /machine:I386 /out:"Debug/wmi.nsm" /pdbtype:sept /libpath:"..\..\..\libnetxms\Debug"
# SUBTRACT LINK32 /debug
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\wmi.nsm ..\..\..\..\bin	copy Debug\wmi.pdb ..\..\..\..\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "wmi - Win32 Release AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "wmi___Win32_Release_AMD64"
# PROP BASE Intermediate_Dir "wmi___Win32_Release_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /D "__64BIT__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib pdh.lib /nologo /dll /machine:I386 /out:"Release/wmi.nsm" /libpath:"..\..\..\libnetxms\Release"
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib wbemuuid.lib /nologo /dll /debug /machine:I386 /out:"Release64/wmi.nsm" /pdbtype:sept /libpath:"..\..\..\libnetxms\Release64" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release64\wmi.nsm C:\NetXMS\bin64	copy Release64\wmi.pdb C:\NetXMS\bin64
# End Special Build Tool

!ELSEIF  "$(CFG)" == "wmi - Win32 Debug AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "wmi___Win32_Debug_AMD64"
# PROP BASE Intermediate_Dir "wmi___Win32_Debug_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /Zi /Od /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "wmi_EXPORTS" /D "__64BIT__" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib pdh.lib /nologo /dll /debug /machine:I386 /out:"Debug/wmi.nsm" /pdbtype:sept /libpath:"..\..\..\libnetxms\Debug"
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib wbemuuid.lib /nologo /dll /debug /machine:I386 /out:"Debug64/wmi.nsm" /pdbtype:sept /libpath:"..\..\..\libnetxms\Debug64" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug64\wmi.nsm ..\..\..\..\bin64	copy Debug64\wmi.pdb ..\..\..\..\bin64
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "wmi - Win32 Release"
# Name "wmi - Win32 Debug"
# Name "wmi - Win32 Release AMD64"
# Name "wmi - Win32 Debug AMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\acpi.cpp
# End Source File
# Begin Source File

SOURCE=.\wmi.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\..\..\include\nms_agent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nms_threads.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nms_util.h
# End Source File
# Begin Source File

SOURCE=.\wmi.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
