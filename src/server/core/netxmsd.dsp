# Microsoft Developer Studio Project File - Name="netxmsd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=netxmsd - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "netxmsd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "netxmsd.mak" CFG="netxmsd - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "netxmsd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "netxmsd - Win32 Debug" (based on "Win32 (x86) Console Application")
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
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libsnmp.lib iphlpapi.lib libnetxms.lib libeay32.lib ssleay32.lib libnxcscp.lib /nologo /subsystem:console /machine:I386 /libpath:"..\..\libnetxms\Release" /libpath:"..\..\libnxcscp\Release"

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
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libsnmp.lib iphlpapi.lib libnetxms.lib libeay32.lib ssleay32.lib libnxcscp.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\..\libnetxms\Debug" /libpath:"..\..\libnxcscp\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy Debug\netxmsd.exe ..\..\..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "netxmsd - Win32 Release"
# Name "netxmsd - Win32 Debug"
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

SOURCE=.\agent.cpp
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

SOURCE=.\datacoll.cpp
# End Source File
# Begin Source File

SOURCE=.\db.cpp
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

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\discovery.cpp
# End Source File
# Begin Source File

SOURCE=.\dload.cpp
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

SOURCE=.\log.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\netinfo.cpp
# End Source File
# Begin Source File

SOURCE=.\netobj.cpp
# End Source File
# Begin Source File

SOURCE=.\node.cpp
# End Source File
# Begin Source File

SOURCE=.\np.cpp
# End Source File
# Begin Source File

SOURCE=.\objects.cpp
# End Source File
# Begin Source File

SOURCE=.\queue.cpp
# End Source File
# Begin Source File

SOURCE=.\session.cpp
# End Source File
# Begin Source File

SOURCE=.\snmp.cpp
# End Source File
# Begin Source File

SOURCE=.\srvroot.cpp
# End Source File
# Begin Source File

SOURCE=.\status.cpp
# End Source File
# Begin Source File

SOURCE=.\subnet.cpp
# End Source File
# Begin Source File

SOURCE=.\syncer.cpp
# End Source File
# Begin Source File

SOURCE=.\tools.cpp
# End Source File
# Begin Source File

SOURCE=.\users.cpp
# End Source File
# Begin Source File

SOURCE=.\watchdog.cpp
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

SOURCE=.\nms_actions.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_agent.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_common.h
# End Source File
# Begin Source File

SOURCE=.\nms_core.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_cscp.h
# End Source File
# Begin Source File

SOURCE=.\nms_dcoll.h
# End Source File
# Begin Source File

SOURCE=.\nms_events.h
# End Source File
# Begin Source File

SOURCE=.\nms_locks.h
# End Source File
# Begin Source File

SOURCE=.\nms_objects.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\nms_threads.h
# End Source File
# Begin Source File

SOURCE=.\nms_users.h
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

SOURCE=.\nxqueue.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\core.rc
# End Source File
# Begin Source File

SOURCE=.\MSG00001.bin
# End Source File
# End Group
# Begin Group "Message files"

# PROP Default_Filter ".mc"
# Begin Source File

SOURCE=.\messages.mc

!IF  "$(CFG)" == "netxmsd - Win32 Release"

# Begin Custom Build - Running Message Compiler on $(InputPath)
ProjDir=.
InputPath=.\messages.mc
InputName=messages

BuildCmds= \
	mc -s -U -h $(ProjDir) -r $(ProjDir) $(InputName) \
	del $(ProjDir)\$(InputName).rc \
	

"$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Msg00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "netxmsd - Win32 Debug"

# Begin Custom Build - Running Message Compiler on $(InputPath)
ProjDir=.
InputPath=.\messages.mc
InputName=messages

BuildCmds= \
	mc -s -U -h $(ProjDir) -r $(ProjDir) $(InputName) \
	del $(ProjDir)\$(InputName).rc \
	

"$(InputName).h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"Msg00001.bin" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
