# Microsoft Developer Studio Project File - Name="libnxsl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libnxsl - Win32 Debug AMD64
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libnxsl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libnxsl.mak" CFG="libnxsl - Win32 Debug AMD64"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libnxsl - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnxsl - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnxsl - Win32 Release AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libnxsl - Win32 Debug AMD64" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libnxsl - Win32 Release"

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
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX- /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcscp.lib ws2_32.lib /nologo /dll /machine:I386 /libpath:"..\libnetxms\Release" /libpath:"..\libnxcscp\Release"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\libnxsl.dll C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Debug"

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
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX- /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcscp.lib ws2_32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\libnetxms\Debug" /libpath:"..\libnxcscp\Debug"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\libnxsl.dll ..\..\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Release AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "libnxsl___Win32_Release_AMD64"
# PROP BASE Intermediate_Dir "libnxsl___Win32_Release_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release64"
# PROP Intermediate_Dir "Release64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX- /O2 /I "..\..\include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /D "__64BIT__" /FD /c
# SUBTRACT CPP /WX /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcscp.lib ws2_32.lib /nologo /dll /machine:I386 /libpath:"..\libnetxms\Release" /libpath:"..\libnxcscp\Release"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcscp.lib ws2_32.lib bufferoverflowU.lib /nologo /dll /machine:I386 /libpath:"..\libnetxms\Release" /libpath:"..\libnxcscp\Release" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Release\libnxsl.dll C:\NetXMS\bin
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Debug AMD64"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libnxsl___Win32_Debug_AMD64"
# PROP BASE Intermediate_Dir "libnxsl___Win32_Debug_AMD64"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug64"
# PROP Intermediate_Dir "Debug64"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX- /Zi /Od /I "..\..\include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBNXSL_EXPORTS" /D "__64BIT__" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcscp.lib ws2_32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\libnetxms\Debug" /libpath:"..\libnxcscp\Debug"
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libnetxms.lib libnxcscp.lib ws2_32.lib bufferoverflowU.lib /nologo /dll /debug /machine:I386 /pdbtype:sept /libpath:"..\libnetxms\Debug" /libpath:"..\libnxcscp\Debug" /machine:AMD64
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Copy files
PostBuild_Cmds=copy Debug\libnxsl.dll ..\..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libnxsl - Win32 Release"
# Name "libnxsl - Win32 Debug"
# Name "libnxsl - Win32 Release AMD64"
# Name "libnxsl - Win32 Debug AMD64"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\class.cpp
# End Source File
# Begin Source File

SOURCE=.\compiler.cpp
# End Source File
# Begin Source File

SOURCE=.\env.cpp
# End Source File
# Begin Source File

SOURCE=.\functions.cpp
# End Source File
# Begin Source File

SOURCE=.\instruction.cpp
# End Source File
# Begin Source File

SOURCE=.\lex.yy.cpp
# End Source File
# Begin Source File

SOURCE=.\lexer.cpp
# End Source File
# Begin Source File

SOURCE=.\library.cpp
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.tab.cpp
# End Source File
# Begin Source File

SOURCE=.\program.cpp
# End Source File
# Begin Source File

SOURCE=.\stack.cpp
# End Source File
# Begin Source File

SOURCE=.\value.cpp
# End Source File
# Begin Source File

SOURCE=.\variable.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\libnxsl.h
# End Source File
# Begin Source File

SOURCE="..\..\include\netxms-regex.h"
# End Source File
# Begin Source File

SOURCE="..\..\include\netxms-version.h"
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

SOURCE=..\..\include\nxcscpapi.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxqueue.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxsl.h
# End Source File
# Begin Source File

SOURCE=..\..\include\nxsl_classes.h
# End Source File
# Begin Source File

SOURCE=.\parser.tab.hpp
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

!IF  "$(CFG)" == "libnxsl - Win32 Release"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.l

"lex.yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -Bf8+ -olex.yy.cpp $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Debug"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.l

"lex.yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -Bf8+ -olex.yy.cpp $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Release AMD64"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.l

"lex.yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -Bf8+ -olex.yy.cpp $(InputPath)

# End Custom Build

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Debug AMD64"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.l

"lex.yy.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	flex -Bf8+ -olex.yy.cpp $(InputPath)

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\parser.y

!IF  "$(CFG)" == "libnxsl - Win32 Release"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.y

BuildCmds= \
	bison -b parser -o parser.tab.cpp -d -t -v $(InputPath)

"parser.tab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.tab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Debug"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.y

BuildCmds= \
	bison -b parser -o parser.tab.cpp -d -t -v $(InputPath)

"parser.tab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.tab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Release AMD64"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.y

BuildCmds= \
	bison -b parser -o parser.tab.cpp -d -t -v $(InputPath)

"parser.tab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.tab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ELSEIF  "$(CFG)" == "libnxsl - Win32 Debug AMD64"

# Begin Custom Build - Compiling $(InputPath)
InputPath=.\parser.y

BuildCmds= \
	bison -b parser -o parser.tab.cpp -d -t -v $(InputPath)

"parser.tab.cpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)

"parser.tab.hpp" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
   $(BuildCmds)
# End Custom Build

!ENDIF 

# End Source File
# End Group
# End Target
# End Project
