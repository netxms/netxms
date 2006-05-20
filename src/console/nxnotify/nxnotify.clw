; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CAlarmPopup
LastTemplate=generic CWnd
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "nxnotify.h"
LastPage=0

ClassCount=10
Class1=CNxnotifyApp
Class3=CChildView
Class4=CMainFrame
Class9=CAboutDlg

ResourceCount=4
Resource1=IDR_MAINFRAME
Resource2=IDM_TASKBAR
Resource3=IDD_ABOUTBOX
Class10=CAlarmPopup
Resource4=IDD_REQUEST_WAIT

[CLS:CNxnotifyApp]
Type=0
HeaderFile=nxnotify.h
ImplementationFile=nxnotify.cpp
Filter=N
BaseClass=CWinApp
VirtualFilter=AC

[CLS:CChildView]
Type=0
HeaderFile=ChildView.h
ImplementationFile=ChildView.cpp
Filter=N

[CLS:CMainFrame]
Type=0
HeaderFile=MainFrm.h
ImplementationFile=MainFrm.cpp
Filter=T
BaseClass=CFrameWnd
VirtualFilter=fWC




[CLS:CAboutDlg]
Type=0
HeaderFile=nxnotify.cpp
ImplementationFile=nxnotify.cpp
Filter=D

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_SETTINGS
Command2=ID_APP_EXIT
Command3=ID_APP_ABOUT
CommandCount=3

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_EDIT_COPY
Command2=ID_EDIT_PASTE
Command3=ID_EDIT_UNDO
Command4=ID_EDIT_CUT
Command5=ID_NEXT_PANE
Command6=ID_PREV_PANE
Command7=ID_EDIT_COPY
Command8=ID_EDIT_PASTE
Command9=ID_EDIT_CUT
Command10=ID_EDIT_UNDO
CommandCount=10

[MNU:IDM_TASKBAR]
Type=1
Class=?
Command1=ID_TASKBAR_OPEN
Command2=ID_FILE_SETTINGS
Command3=ID_APP_ABOUT
Command4=ID_APP_EXIT
CommandCount=4

[DLG:IDD_REQUEST_WAIT]
Type=1
Class=?
ControlCount=2
Control1=IDC_STATIC,static,1342177283
Control2=IDC_INFO_TEXT,static,1342308352

[CLS:CAlarmPopup]
Type=0
HeaderFile=AlarmPopup.h
ImplementationFile=AlarmPopup.cpp
BaseClass=CTaskBarPopupWnd
Filter=W
VirtualFilter=WC

