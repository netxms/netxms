# Microsoft Developer Studio Project File - Name="nxav" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=nxav - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nxav.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nxav.mak" CFG="nxav - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nxav - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "nxav - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "nxav - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 libnetxms.lib libnxcl.lib nxuilib.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\..\libnetxms\Release" /libpath:"..\..\libnxcl\Release" /libpath:"..\nxuilib\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\nxav.exe C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxav - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libnetxms.lib libnxcl.lib nxuilib.lib "C:\Program Files\Microsoft Visual Studio\VC98\Lib\uuid.lib" /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"uuid.lib" /pdbtype:sept /libpath:"..\..\libnetxms\Debug" /libpath:"..\..\libnxcl\Debug" /libpath:"..\nxuilib\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\nxav.exe ..\..\..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "nxav - Win32 Release"
# Name "nxav - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AlarmBrowser.cpp
# End Source File
# Begin Source File

SOURCE=.\comm.cpp
# End Source File
# Begin Source File

SOURCE=.\globals.cpp
# End Source File
# Begin Source File

SOURCE=.\InfoLine.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\nxav.cpp
# End Source File
# Begin Source File

SOURCE=.\nxav.rc
# End Source File
# Begin Source File

SOURCE=.\RequestProcessingDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\SettingsDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\tools.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AlarmBrowser.h
# End Source File
# Begin Source File

SOURCE=.\globals.h
# End Source File
# Begin Source File

SOURCE=.\InfoLine.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_util.h
# End Source File
# Begin Source File

SOURCE=.\nxav.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxclapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxwinui.h
# End Source File
# Begin Source File

SOURCE=.\RequestProcessingDlg.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\SettingsDlg.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\ack.png
# End Source File
# Begin Source File

SOURCE=.\res\background.jpg
# End Source File
# Begin Source File

SOURCE=.\res\critical.ico
# End Source File
# Begin Source File

SOURCE=.\res\major.ico
# End Source File
# Begin Source File

SOURCE=.\res\minor.ico
# End Source File
# Begin Source File

SOURCE=.\res\normal.ico
# End Source File
# Begin Source File

SOURCE=.\res\nxav.ico
# End Source File
# Begin Source File

SOURCE=.\res\nxav.rc2
# End Source File
# Begin Source File

SOURCE=.\res\processing.ico
# End Source File
# Begin Source File

SOURCE=.\res\SeverityCritical.ico
# End Source File
# Begin Source File

SOURCE=.\res\SeverityMajor.ico
# End Source File
# Begin Source File

SOURCE=.\res\SeverityMinor.ico
# End Source File
# Begin Source File

SOURCE=.\res\SeverityNormal.ico
# End Source File
# Begin Source File

SOURCE=.\res\SeverityWarning.ico
# End Source File
# Begin Source File

SOURCE=.\res\small_logo.bmp
# End Source File
# Begin Source File

SOURCE=.\res\warning.ico
# End Source File
# End Group
# Begin Source File

SOURCE=.\res\nosound.png
# End Source File
# Begin Source File

SOURCE=.\res\sound.png
# End Source File
# End Target
# End Project
