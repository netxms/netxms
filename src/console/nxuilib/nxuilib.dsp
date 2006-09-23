# Microsoft Developer Studio Project File - Name="nxuilib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=nxuilib - Win32 Debug UNICODE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nxuilib.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nxuilib.mak" CFG="nxuilib - Win32 Debug UNICODE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nxuilib - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "nxuilib - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "nxuilib - Win32 Debug UNICODE" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "nxuilib - Win32 Release UNICODE" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "nxuilib - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "NXUILIB_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /machine:I386
# ADD LINK32 libnetxms.lib libnxcl.lib sapi.lib winmm.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\libnetxms\Release" /libpath:"..\..\libnxcl\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\nxuilib.dll C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxuilib - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "NXUILIB_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libnetxms.lib libnxcl.lib sapi.lib winmm.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\libnetxms\Debug" /libpath:"..\..\libnxcl\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\nxuilib.dll ..\..\..\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxuilib - Win32 Debug UNICODE"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "nxuilib___Win32_Debug_UNICODE"
# PROP BASE Intermediate_Dir "nxuilib___Win32_Debug_UNICODE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_UNICODE"
# PROP Intermediate_Dir "Debug_UNICODE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "NXUILIB_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\..\include" /D "UNICODE" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "NXUILIB_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /fo"Debug_UNICODE/nxuilibw.res" /d "_DEBUG" /d "_AFXDLL" /d "UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libnetxms.lib libnxcl.lib sapi.lib winmm.lib /nologo /subsystem:windows /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\libnetxms\Debug" /libpath:"..\..\libnxcl\Debug"
# ADD LINK32 libnetxmsw.lib libnxclw.lib sapi.lib winmm.lib /nologo /subsystem:windows /dll /debug /machine:I386 /def:".\nxuilibw.def" /out:"Debug_UNICODE/nxuilibw.dll" /pdbtype:sept /libpath:"..\..\libnetxms\Debug_UNICODE" /libpath:"..\..\libnxcl\Debug_UNICODE"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug_UNICODE\nxuilibw.dll ..\..\..\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxuilib - Win32 Release UNICODE"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nxuilib___Win32_Release_UNICODE"
# PROP BASE Intermediate_Dir "nxuilib___Win32_Release_UNICODE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_UNICODE"
# PROP Intermediate_Dir "Release_UNICODE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O2 /I "..\..\..\include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "NXUILIB_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\..\include" /D "UNICODE" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_MBCS" /D "_AFXEXT" /D "NXUILIB_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x409 /fo"Release_UNICODE/nxuilibw.res" /d "NDEBUG" /d "_AFXDLL" /d "UNICODE"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libnetxms.lib libnxcl.lib sapi.lib winmm.lib /nologo /subsystem:windows /dll /machine:I386 /libpath:"..\..\libnetxms\Release" /libpath:"..\..\libnxcl\Release"
# ADD LINK32 libnetxmsw.lib libnxclw.lib sapi.lib winmm.lib /nologo /subsystem:windows /dll /machine:I386 /def:".\nxuilibw.def" /out:"Release_UNICODE/nxuilibw.dll" /libpath:"..\..\libnetxms\Release_UNICODE" /libpath:"..\..\libnxcl\Release_UNICODE"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release_UNICODE\nxuilibw.dll C:\NetXMS\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "nxuilib - Win32 Release"
# Name "nxuilib - Win32 Debug"
# Name "nxuilib - Win32 Debug UNICODE"
# Name "nxuilib - Win32 Release UNICODE"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AlarmSoundDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\FlatButton.cpp
# End Source File
# Begin Source File

SOURCE=.\LoginDialog.cpp
# End Source File
# Begin Source File

SOURCE=.\nxuilib.cpp
# End Source File
# Begin Source File

SOURCE=.\nxuilib.def

!IF  "$(CFG)" == "nxuilib - Win32 Release"

!ELSEIF  "$(CFG)" == "nxuilib - Win32 Debug"

!ELSEIF  "$(CFG)" == "nxuilib - Win32 Debug UNICODE"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "nxuilib - Win32 Release UNICODE"

# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\nxuilib.rc
# End Source File
# Begin Source File

SOURCE=.\ScintillaCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\SimpleListCtrl.cpp
# End Source File
# Begin Source File

SOURCE=.\speaker.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TaskBarPopupWnd.cpp
# End Source File
# Begin Source File

SOURCE=.\tools.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AlarmSoundDlg.h
# End Source File
# Begin Source File

SOURCE=.\FlatButton.h
# End Source File
# Begin Source File

SOURCE=.\LoginDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxlexer_styles.h
# End Source File
# Begin Source File

SOURCE=.\nxuilib.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nxwinui.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\ScintillaCtrl.h
# End Source File
# Begin Source File

SOURCE=.\SimpleListCtrl.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TaskBarPopupWnd.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\sounds\alarm1.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\alarm2.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\beep.wav
# End Source File
# Begin Source File

SOURCE=.\res\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\res\login.bmp
# End Source File
# Begin Source File

SOURCE=.\sounds\misc1.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\misc2.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\misc3.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\misc4.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\misc5.wav
# End Source File
# Begin Source File

SOURCE=.\res\nxuilib.rc2
# End Source File
# Begin Source File

SOURCE=.\res\play.ico
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\sounds\ring1.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\ring2.wav
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

SOURCE=.\sounds\siren1.wav
# End Source File
# Begin Source File

SOURCE=.\sounds\siren2.wav
# End Source File
# End Group
# End Target
# End Project
