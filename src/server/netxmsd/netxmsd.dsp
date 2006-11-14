# Microsoft Developer Studio Project File - Name="netxmsd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=netxmsd - Win32 Debug AMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "netxmsd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "netxmsd.mak" CFG="netxmsd - Win32 Debug AMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "netxmsd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "netxmsd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE "netxmsd - Win32 Release AMD64" (based on "Win32 (x86) Console Application")
!MESSAGE "netxmsd - Win32 Debug AMD64" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "netxmsd - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\include" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib nxcore.lib libnxsrv.lib /nologo /subsystem:console /machine:I386 /libpath:"..\core\Release" /libpath:"..\libnxsrv\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\netxmsd.exe C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "netxmsd - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\include" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib nxcore.lib libnxsrv.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\core\Debug" /libpath:"..\libnxsrv\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\netxmsd.exe ..\..\..\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "netxmsd - Win32 Release AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "netxmsd___Win32_Release_AMD64"
# PROP BASE Intermediate_Dir "netxmsd___Win32_Release_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\include" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\include" /I "..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__64BIT__" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib nxcore.lib /nologo /subsystem:console /machine:I386 /libpath:"..\core\Release"
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib nxcore.lib libnxsrv.lib /nologo /subsystem:console /machine:I386 /libpath:"..\core\Release64" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release64\netxmsd.exe C:\NetXMS\bin64
# End Special Build Tool

!ELSEIF  "$(CFG)" == "netxmsd - Win32 Debug AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "netxmsd___Win32_Debug_AMD64"
# PROP BASE Intermediate_Dir "netxmsd___Win32_Debug_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 1
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /Zi /Od /I "..\include" /I "..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "__64BIT__" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib nxcore.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\core\Debug"
# ADD LINK32 bufferoverflowU.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib nxcore.lib libnxsrv.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\core\Debug64" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug64\netxmsd.exe ..\..\..\bin64
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "netxmsd - Win32 Release"
# Name "netxmsd - Win32 Debug"
# Name "netxmsd - Win32 Release AMD64"
# Name "netxmsd - Win32 Debug AMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\netxmsd.cpp
# End Source File
# Begin Source File

SOURCE=.\winsrv.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\dbdrv.h
# End Source File
# Begin Source File

SOURCE=..\include\local_admin.h
# End Source File
# Begin Source File

SOURCE=..\include\messages.h
# End Source File
# Begin Source File

SOURCE="..\..\..\include\netxms-version.h"
# End Source File
# Begin Source File

SOURCE=.\netxmsd.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\netxmsdb.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_actions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_agent.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_alarm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_common.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_core.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_cscp.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_dcoll.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_events.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_locks.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_objects.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_threads.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_users.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxclapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxcscpapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxevent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nximage.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxnt.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxsnmp.h
# End Source File
# Begin Source File

SOURCE=..\include\nxsrvapi.h
# End Source File
# Begin Source File

SOURCE=..\include\rwlock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\unicode.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
