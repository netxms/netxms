; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CConnectionPage
LastTemplate=CPropertyPage
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "nxnotify.h"
LastPage=0

ClassCount=7
Class1=CNxnotifyApp
Class3=CMainFrame
Class4=CAboutDlg

ResourceCount=7
Resource1=IDD_ABOUTBOX
Resource2=IDM_TASKBAR
Resource3=IDM_CONTEXT
Resource4=IDD_CONNECTION_PAGE
Resource5=IDD_REQUEST_WAIT
Resource6=IDD_POUP_CFG_PAGE
Class2=CChildView
Class5=CAlarmPopup
Class6=CConnectionPage
Class7=CPopupCfgPage
Resource7=IDR_MAINFRAME

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
BaseClass=CDialog
VirtualFilter=dWC

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC_VERSION_INFO,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_SETTINGS
Command2=ID_FILE_EXIT
Command3=ID_APP_ABOUT
CommandCount=3

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_APP_ABOUT
Command2=ID_FILE_SETTINGS
Command3=ID_FILE_EXIT
CommandCount=3

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
LastObject=CAlarmPopup

[MNU:IDM_CONTEXT]
Type=1
Class=?
Command1=ID_TASKBAR_OPEN
Command2=ID_FILE_SETTINGS
Command3=ID_APP_ABOUT
Command4=ID_FILE_EXIT
CommandCount=4

[DLG:IDD_POUP_CFG_PAGE]
Type=1
Class=CPopupCfgPage
ControlCount=12
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_TIMEOUT,edit,1350631552
Control3=IDC_STATIC,static,1342308352
Control4=IDC_COMBO_LEFT,combobox,1344339971
Control5=IDC_STATIC,static,1342308352
Control6=IDC_COMBO_RIGHT,combobox,1344339971
Control7=IDC_STATIC,static,1342308352
Control8=IDC_COMBO_DBLCLK,combobox,1344339971
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_EDIT_SOUND,edit,1350633600
Control12=IDC_CHANGE_SOUND,button,1342242816

[DLG:IDD_CONNECTION_PAGE]
Type=1
Class=CConnectionPage
ControlCount=11
Control1=IDC_CHECK_AUTOLOGIN,button,1342242819
Control2=IDC_SERVER,edit,1350631552
Control3=IDC_LOGIN,edit,1350631552
Control4=IDC_PASSWORD,edit,1350631552
Control5=IDC_ENCRYPT,button,1342242819
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342177283
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352

[CLS:CConnectionPage]
Type=0
HeaderFile=ConnectionPage.h
ImplementationFile=ConnectionPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CConnectionPage

[CLS:CPopupCfgPage]
Type=0
HeaderFile=PopupCfgPage.h
ImplementationFile=PopupCfgPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC

