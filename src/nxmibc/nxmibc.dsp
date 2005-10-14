# Microsoft Developer Studio Project File - Name="nxmibc" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=nxmibc - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nxmibc.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nxmibc.mak" CFG="nxmibc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nxmibc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "nxmibc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "nxmibc - Win32 Release"

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
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxsnmp.lib ws2_32.lib /nologo /subsystem:console /machine:I386 /libpath:"..\libnetxms\Release" /libpath:"..\libnxsnmp\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\nxmibc.exe C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "nxmibc - Win32 Debug"

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
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxsnmp.lib ws2_32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept /libpath:"..\libnetxms\Debug" /libpath:"..\libnxsnmp\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\nxmibc.exe ..\..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "nxmibc - Win32 Release"
# Name "nxmibc - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\array.cpp
# End Source File
# Begin Source File

SOURCE=.\lex.mp.c
# End Source File
# Begin Source File

SOURCE=.\mibparse.cpp
# End Source File
# Begin Source File

SOURCE=.\nxmibc.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.tab.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\mibparse.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_common.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_threads.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nms_util.h
# End Source File
# Begin Source File

SOURCE=.\nxmibc.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxsnmp.h
# End Source File
# Begin Source File

SOURCE=.\parser.tab.h
# End Source File
# Begin Source File

SOURCE=..\..\include\unicode.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Group "Parser Files"

# PROP Default_Filter "l;y"
# Begin Source File

SOURCE=.\parser.l

!IF  "$(CFG)" == "nxmibc - Win32 Release"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.l

"lex.mp.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -Pmp $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "nxmibc - Win32 Debug"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.l

"lex.mp.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -Pmp $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parser.y

!IF  "$(CFG)" == "nxmibc - Win32 Release"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.y

BuildCmds= \
	bison -d -p mp -t -v $(InputPath)

"parser.tab.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.tab.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "nxmibc - Win32 Debug"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.y

BuildCmds= \
	bison -d -p mp -t -v $(InputPath)

"parser.tab.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.tab.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
