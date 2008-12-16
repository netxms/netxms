# Microsoft Developer Studio Project File - Name="nxhttpd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=nxhttpd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nxhttpd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nxhttpd.mak" CFG="nxhttpd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nxhttpd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "nxhttpd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "nxhttpd - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcl.lib ws2_32.lib bgd.lib /nologo /subsystem:console /machine:I386 /libpath:"..\libnetxms\Release" /libpath:"..\libnxcl\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\nxhttpd.exe C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxhttpd - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcl.lib ws2_32.lib bgd.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\libnetxms\Debug" /libpath:"..\libnxcl\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\nxhttpd.exe ..\..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "nxhttpd - Win32 Release"
# Name "nxhttpd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\alarm.cpp
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrlpanel.cpp
# End Source File
# Begin Source File

SOURCE=.\dataview.cpp
# End Source File
# Begin Source File

SOURCE=.\html.cpp
# End Source File
# Begin Source File

SOURCE=.\httprequest.cpp
# End Source File
# Begin Source File

SOURCE=.\httpresponse.cpp
# End Source File
# Begin Source File

SOURCE=.\json.cpp
# End Source File
# Begin Source File

SOURCE=.\nxhttpd.cpp
# End Source File
# Begin Source File

SOURCE=.\object.cpp
# End Source File
# Begin Source File

SOURCE=.\piechart.cpp
# End Source File
# Begin Source File

SOURCE=.\session.cpp
# End Source File
# Begin Source File

SOURCE=.\smgr.cpp
# End Source File
# Begin Source File

SOURCE=.\tools.cpp
# End Source File
# Begin Source File

SOURCE=.\winsrv.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\messages.h
# End Source File
# Begin Source File

SOURCE="..\..\include\netxms-version.h"
# End Source File
# Begin Source File

SOURCE=..\..\include\netxms_getopt.h
# End Source File
# Begin Source File

SOURCE=..\..\include\netxms_maps.h
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

SOURCE=..\..\include\nxclapi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxcpapi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxevent.h
# End Source File
# Begin Source File

SOURCE=.\nxhttpd.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxlog.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\nxhttpd.rc
# End Source File
# End Group
# Begin Group "Message FIles"

# PROP Default_Filter "mc"
# Begin Source File

SOURCE=.\messages.mc

!IF  "$(CFG)" == "nxhttpd - Win32 Release"

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

!ELSEIF  "$(CFG)" == "nxhttpd - Win32 Debug"

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
