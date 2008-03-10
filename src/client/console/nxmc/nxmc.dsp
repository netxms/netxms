# Microsoft Developer Studio Project File - Name="nxmc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=nxmc - Win32 Debug UNICODE
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nxmc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nxmc.mak" CFG="nxmc - Win32 Debug UNICODE"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nxmc - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "nxmc - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "nxmc - Win32 Release UNICODE" (based on "Win32 (x86) Application")
!MESSAGE "nxmc - Win32 Debug UNICODE" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "nxmc - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\..\..\libnetxms\Release"

!ELSEIF  "$(CFG)" == "nxmc - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wxbase28ud.lib wxmsw28ud_core.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\libnetxms\Debug"

!ELSEIF  "$(CFG)" == "nxmc - Win32 Release UNICODE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "nxmc___Win32_Release_UNICODE"
# PROP BASE Intermediate_Dir "nxmc___Win32_Release_UNICODE"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_UNICODE"
# PROP Intermediate_Dir "Release_UNICODE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\include" /I "..\..\..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "UNICODE" /D "__WXMSW__" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 libnetxmsw.lib libnxclw.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libnxsnmpw.lib /nologo /subsystem:windows /machine:I386 /libpath:"..\..\..\libnetxms\Release_UNICODE" /libpath:"..\..\..\libnxcl\Release_UNICODE" /libpath:"..\..\..\libnxsnmp\Release_UNICODE"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release_UNICODE\nxmc.exe C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxmc - Win32 Debug UNICODE"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "nxmc___Win32_Debug_UNICODE"
# PROP BASE Intermediate_Dir "nxmc___Win32_Debug_UNICODE"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_UNICODE"
# PROP Intermediate_Dir "Debug_UNICODE"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\..\..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "UNICODE" /D "__WXMSW__" /D "__WXDEBUG__" /D WXDEBUG=1 /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib wxbase28ud.lib wxmsw28ud_core.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wxbase28ud.lib wxmsw28ud_core.lib libnetxmsw.lib libnxclw.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib libnxsnmpw.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept /libpath:"..\..\..\libnetxms\Debug_UNICODE" /libpath:"..\..\..\libnxcl\Debug_UNICODE" /libpath:"..\..\..\libnxsnmp\Debug_UNICODE"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug_UNICODE\nxmc.exe ..\..\..\..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "nxmc - Win32 Release"
# Name "nxmc - Win32 Debug"
# Name "nxmc - Win32 Release UNICODE"
# Name "nxmc - Win32 Debug UNICODE"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\busydlg.cpp
# End Source File
# Begin Source File

SOURCE=.\comm.cpp
# End Source File
# Begin Source File

SOURCE=.\conlog.cpp
# End Source File
# Begin Source File

SOURCE=.\ctrlpanel.cpp
# End Source File
# Begin Source File

SOURCE=.\frame.cpp
# End Source File
# Begin Source File

SOURCE=.\logindlg.cpp
# End Source File
# Begin Source File

SOURCE=.\mainfrm.cpp
# End Source File
# Begin Source File

SOURCE=.\nxmc.cpp
# End Source File
# Begin Source File

SOURCE=.\plugins.cpp
# End Source File
# Begin Source File

SOURCE=.\sound.cpp
# End Source File
# Begin Source File

SOURCE=.\srvcfg.cpp
# End Source File
# Begin Source File

SOURCE=.\tbicon.cpp
# End Source File
# Begin Source File

SOURCE=.\vareditdlg.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\busydlg.h
# End Source File
# Begin Source File

SOURCE=.\conlog.h
# End Source File
# Begin Source File

SOURCE=.\ctrlpanel.h
# End Source File
# Begin Source File

SOURCE=.\frame.h
# End Source File
# Begin Source File

SOURCE=.\logindlg.h
# End Source File
# Begin Source File

SOURCE=.\mainfrm.h
# End Source File
# Begin Source File

SOURCE="..\..\..\..\include\netxms-version.h"
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\netxms_maps.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nms_common.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nms_cscp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nms_threads.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nms_util.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nxclapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\..\include\nxcpapi.h
# End Source File
# Begin Source File

SOURCE=.\nxmc.h
# End Source File
# Begin Source File

SOURCE=..\include\nxmc_api.h
# End Source File
# Begin Source File

SOURCE=.\sound.h
# End Source File
# Begin Source File

SOURCE=.\srvcfg.h
# End Source File
# Begin Source File

SOURCE=.\tbicon.h
# End Source File
# Begin Source File

SOURCE=.\vareditdlg.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\rc\manual.xrc
# End Source File
# Begin Source File

SOURCE=.\rc\nxmc.ico
# End Source File
# Begin Source File

SOURCE=.\nxmc.rc
# End Source File
# Begin Source File

SOURCE=.\rc\nxmc.xrs
# End Source File
# Begin Source File

SOURCE=.\rc\wxfb_code.xrc

!IF  "$(CFG)" == "nxmc - Win32 Release"

USERDEP__WXFB_="rc\manual.xrc"	
# Begin Custom Build - Processing $(InputPath)
ProjDir=.
InputPath=.\rc\wxfb_code.xrc

"rc\nxmc.xrs" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ..\..\..\..\tools\update_xrc.pl < $(InputPath) > $(ProjDir)\rc\resource.xrc 
	wxrc $(ProjDir)\rc\resource.xrc $(ProjDir)\rc\manual.xrc -o $(ProjDir)\rc\nxmc.xrs 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "nxmc - Win32 Debug"

USERDEP__WXFB_="rc\manual.xrc"	
# Begin Custom Build - Processing $(InputPath)
ProjDir=.
InputPath=.\rc\wxfb_code.xrc

"rc\nxmc.xrs" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ..\..\..\..\tools\update_xrc.pl < $(InputPath) > $(ProjDir)\rc\resource.xrc 
	wxrc $(ProjDir)\rc\resource.xrc $(ProjDir)\rc\manual.xrc -o $(ProjDir)\rc\nxmc.xrs 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "nxmc - Win32 Release UNICODE"

USERDEP__WXFB_="rc\manual.xrc"	
# Begin Custom Build - Processing $(InputPath)
ProjDir=.
InputPath=.\rc\wxfb_code.xrc

"rc\nxmc.xrs" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ..\..\..\..\tools\update_xrc.pl < $(InputPath) > $(ProjDir)\rc\resource.xrc 
	wxrc $(ProjDir)\rc\resource.xrc $(ProjDir)\rc\manual.xrc -o $(ProjDir)\rc\nxmc.xrs 
	
# End Custom Build

!ELSEIF  "$(CFG)" == "nxmc - Win32 Debug UNICODE"

USERDEP__WXFB_="rc\manual.xrc"	
# Begin Custom Build - Processing $(InputPath)
ProjDir=.
InputPath=.\rc\wxfb_code.xrc

"rc\nxmc.xrs" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	perl ..\..\..\..\tools\update_xrc.pl < $(InputPath) > $(ProjDir)\rc\resource.xrc 
	wxrc $(ProjDir)\rc\resource.xrc $(ProjDir)\rc\manual.xrc -o $(ProjDir)\rc\nxmc.xrs 
	
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
