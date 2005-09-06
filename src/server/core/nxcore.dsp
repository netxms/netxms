# Microsoft Developer Studio Project File - Name="nxcore" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=nxcore - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nxcore.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nxcore.mak" CFG="nxcore - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nxcore - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "nxcore - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "nxcore - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NXCORE_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NXCORE_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libnetxms.lib libnxcscp.lib libnxsrv.lib libnxsnmp.lib iphlpapi.lib libeay32.lib /nologo /dll /machine:I386 /libpath:"..\..\libnetxms\Release" /libpath:"..\..\libnxcscp\Release" /libpath:"..\..\libnxsnmp\Release" /libpath:"..\libnxsrv\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\nxcore.dll C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxcore - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NXCORE_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "NXCORE_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libnetxms.lib libnxcscp.lib libnxsrv.lib libnxsnmp.lib iphlpapi.lib libeay32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\..\libnetxms\Debug" /libpath:"..\..\libnxcscp\Debug" /libpath:"..\..\libnxsnmp\Debug" /libpath:"..\libnxsrv\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\nxcore.dll ..\..\..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "nxcore - Win32 Release"
# Name "nxcore - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\acl.cpp
# End Source File
# Begin Source File

SOURCE=.\actions.cpp
# End Source File
# Begin Source File

SOURCE=.\admin.cpp
# End Source File
# Begin Source File

SOURCE=.\alarm.cpp
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\config.cpp
# End Source File
# Begin Source File

SOURCE=.\container.cpp
# End Source File
# Begin Source File

SOURCE=.\correlate.cpp
# End Source File
# Begin Source File

SOURCE=.\datacoll.cpp
# End Source File
# Begin Source File

SOURCE=.\dbwrite.cpp
# End Source File
# Begin Source File

SOURCE=.\dcitem.cpp
# End Source File
# Begin Source File

SOURCE=.\dcithreshold.cpp
# End Source File
# Begin Source File

SOURCE=.\dcivalue.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\email.cpp
# End Source File
# Begin Source File

SOURCE=.\entirenet.cpp
# End Source File
# Begin Source File

SOURCE=.\epp.cpp
# End Source File
# Begin Source File

SOURCE=.\events.cpp
# End Source File
# Begin Source File

SOURCE=.\evproc.cpp
# End Source File
# Begin Source File

SOURCE=.\hk.cpp
# End Source File
# Begin Source File

SOURCE=.\id.cpp
# End Source File
# Begin Source File

SOURCE=.\image.cpp
# End Source File
# Begin Source File

SOURCE=.\interface.cpp
# End Source File
# Begin Source File

SOURCE=.\locks.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\modules.cpp
# End Source File
# Begin Source File

SOURCE=.\netinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\netobj.cpp
# End Source File
# Begin Source File

SOURCE=.\netsrv.cpp
# End Source File
# Begin Source File

SOURCE=.\node.cpp
# End Source File
# Begin Source File

SOURCE=.\nortel.cpp
# End Source File
# Begin Source File

SOURCE=.\np.cpp
# End Source File
# Begin Source File

SOURCE=.\objects.cpp
# End Source File
# Begin Source File

SOURCE=.\objtools.cpp
# End Source File
# Begin Source File

SOURCE=.\package.cpp
# End Source File
# Begin Source File

SOURCE=.\poll.cpp
# End Source File
# Begin Source File

SOURCE=.\rootobj.cpp
# End Source File
# Begin Source File

SOURCE=.\session.cpp
# End Source File
# Begin Source File

SOURCE=.\sms.cpp
# End Source File
# Begin Source File

SOURCE=.\snmp.cpp
# End Source File
# Begin Source File

SOURCE=.\snmptrap.cpp
# End Source File
# Begin Source File

SOURCE=.\subnet.cpp
# End Source File
# Begin Source File

SOURCE=.\syncer.cpp
# End Source File
# Begin Source File

SOURCE=.\template.cpp
# End Source File
# Begin Source File

SOURCE=.\tools.cpp
# End Source File
# Begin Source File

SOURCE=.\tracert.cpp
# End Source File
# Begin Source File

SOURCE=.\uniroot.cpp
# End Source File
# Begin Source File

SOURCE=.\users.cpp
# End Source File
# Begin Source File

SOURCE=.\vpnconn.cpp
# End Source File
# Begin Source File

SOURCE=.\watchdog.cpp
# End Source File
# Begin Source File

SOURCE=.\zone.cpp
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

SOURCE=..\include\nms_pkg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_threads.h
# End Source File
# Begin Source File

SOURCE=..\include\nms_topo.h
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

SOURCE=.\nxcore.h
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

SOURCE=..\include\nxmodule.h
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

SOURCE=..\..\..\include\nxtools.h
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
