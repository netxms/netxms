; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CToolBox
LastTemplate=generic CWnd
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "nxcon.h"
LastPage=0

ClassCount=18
Class1=CConsoleApp
Class3=CMainFrame
Class4=CChildFrame
Class7=CEventBrowser
Class9=CMapView

ResourceCount=22
Resource1=IDM_VIEW_SPECIFIC (English (U.S.))
Resource2=IDR_MAINFRAME (English (U.S.))
Resource3=IDD_OBJECT_PROPERTIES
Resource4=IDD_ABOUTBOX
Resource5=IDA_OBJECT_BROWSER
Class2=CChildView
Class5=CAboutDlg
Class6=CControlPanel
Class8=CMapFrame
Class10=CLoginDialog
Resource6=IDA_MDI_DEFAULT
Class11=CProgressDialog
Resource7=IDD_PROGRESS (English (U.S.))
Class12=CObjectBrowser
Resource8=IDD_PROGRESS
Class13=CObjectPropDlg
Resource9=IDM_VIEW_SPECIFIC
Resource10=IDR_MAINFRAME
Resource11=IDD_ABOUTBOX (English (U.S.))
Resource12=IDR_CTRLPANEL (English (U.S.))
Resource13=IDR_EVENTS (English (U.S.))
Resource14=IDR_MAPFRAME (English (U.S.))
Resource15=IDR_OBJECTS (English (U.S.))
Resource16=IDD_DUMMY (English (U.S.))
Class14=CEventEditor
Class15=CEditEventDlg
Resource17=IDD_LOGIN
Class16=CDebugFrame
Resource18=IDD_EDIT_EVENT
Resource19=IDA_MDI_DEFAULT (English (U.S.))
Resource20=IDD_OBJECT_PROPERTIES (English (U.S.))
Class17=CObjectPreview
Resource21=IDD_LOGIN (English (U.S.))
Class18=CToolBox
Resource22=IDD_EDIT_EVENT (English (U.S.))

[CLS:CConsoleApp]
Type=0
HeaderFile=nxcon.h
ImplementationFile=nxcon.cpp
Filter=M
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
BaseClass=CMDIFrameWnd
VirtualFilter=fWC


[CLS:CChildFrame]
Type=0
HeaderFile=ChildFrm.h
ImplementationFile=ChildFrm.cpp
Filter=M


[CLS:CAboutDlg]
Type=0
HeaderFile=nxcon.cpp
ImplementationFile=nxcon.cpp
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
Command1=ID_FILE_NEW
Command2=ID_APP_EXIT
Command3=ID_VIEW_MAP
Command4=ID_VIEW_OBJECTBROWSER
Command5=ID_VIEW_EVENTS
Command6=ID_VIEW_CONTROLPANEL
Command7=ID_VIEW_DEBUG
Command8=ID_VIEW_TOOLBAR
Command9=ID_VIEW_STATUS_BAR
Command10=ID_VIEW_REFRESH
Command11=ID_CONTROLPANEL_EVENTS
Command12=ID_CONTROLPANEL_USERS
Command13=ID_APP_ABOUT
CommandCount=13

[TB:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_NEW
Command2=ID_EDIT_CUT
Command3=ID_EDIT_COPY
Command4=ID_EDIT_PASTE
Command5=ID_FILE_PRINT
Command6=ID_APP_ABOUT
CommandCount=6

[ACL:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_EDIT_COPY
Command2=ID_FILE_NEW
Command3=ID_EDIT_PASTE
Command4=ID_EDIT_UNDO
Command5=ID_EDIT_CUT
Command6=ID_VIEW_MAP
Command7=ID_NEXT_PANE
Command8=ID_PREV_PANE
Command9=ID_VIEW_EVENTS
Command10=ID_VIEW_CONTROLPANEL
Command11=ID_EDIT_COPY
Command12=ID_EDIT_PASTE
Command13=ID_EDIT_CUT
Command14=ID_EDIT_UNDO
CommandCount=14

[CLS:CControlPanel]
Type=0
HeaderFile=ControlPanel.h
ImplementationFile=ControlPanel.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC

[CLS:CEventBrowser]
Type=0
HeaderFile=EventBrowser.h
ImplementationFile=EventBrowser.cpp
BaseClass=CMDIChildWnd
Filter=W
VirtualFilter=mfWC

[CLS:CMapFrame]
Type=0
HeaderFile=MapFrame.h
ImplementationFile=MapFrame.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC

[CLS:CMapView]
Type=0
HeaderFile=MapView.h
ImplementationFile=MapView.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC
LastObject=CMapView

[DLG:IDD_LOGIN]
Type=1
Class=CLoginDialog
ControlCount=11
Control1=IDC_EDIT_SERVER,edit,1350631552
Control2=IDC_EDIT_LOGIN,edit,1350631552
Control3=IDC_EDIT_PASSWORD,edit,1350631584
Control4=IDOK,button,1342242817
Control5=IDCANCEL,button,1342242816
Control6=IDC_STATIC,static,1342177294
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342177296
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352

[CLS:CLoginDialog]
Type=0
HeaderFile=LoginDialog.h
ImplementationFile=LoginDialog.cpp
BaseClass=CDialog
Filter=D
LastObject=CLoginDialog
VirtualFilter=dWC

[DLG:IDD_PROGRESS]
Type=1
Class=CProgressDialog
ControlCount=2
Control1=IDC_STATIC_TITLE,button,1342177287
Control2=IDC_STATIC_TEXT,static,1342308865

[CLS:CProgressDialog]
Type=0
HeaderFile=ProgressDialog.h
ImplementationFile=ProgressDialog.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_STATIC_TEXT

[CLS:CObjectBrowser]
Type=0
HeaderFile=ObjectBrowser.h
ImplementationFile=ObjectBrowser.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC
LastObject=CObjectBrowser

[DLG:IDD_OBJECT_PROPERTIES]
Type=1
Class=CObjectPropDlg
ControlCount=1
Control1=IDC_LIST_VIEW,SysListView32,1342275613

[CLS:CObjectPropDlg]
Type=0
HeaderFile=ObjectPropDlg.h
ImplementationFile=ObjectPropDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CObjectPropDlg

[TB:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_EDIT_CUT
Command3=ID_EDIT_COPY
Command4=ID_EDIT_PASTE
Command5=ID_FILE_PRINT
Command6=ID_APP_ABOUT
CommandCount=6

[MNU:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_SETTINGS
Command2=ID_FILE_EXPORT
Command3=ID_FILE_PAGESETUP
Command4=ID_FILE_PRINT
Command5=ID_APP_EXIT
Command6=ID_VIEW_MAP
Command7=ID_VIEW_OBJECTBROWSER
Command8=ID_VIEW_EPP
Command9=ID_VIEW_EVENTS
Command10=ID_VIEW_CONTROLPANEL
Command11=ID_VIEW_DEBUG
Command12=ID_VIEW_TOOLBAR
Command13=ID_VIEW_STATUS_BAR
Command14=ID_VIEW_REFRESH
Command15=ID_CONTROLPANEL_EVENTS
Command16=ID_CONTROLPANEL_USERS
Command17=ID_APP_ABOUT
CommandCount=17

[MNU:IDR_CTRLPANEL (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_CLOSE
Command3=ID_APP_EXIT
Command4=ID_VIEW_MAP
Command5=ID_VIEW_OBJECTBROWSER
Command6=ID_VIEW_EVENTS
Command7=ID_VIEW_CONTROLPANEL
Command8=ID_VIEW_TOOLBAR
Command9=ID_VIEW_STATUS_BAR
Command10=ID_WINDOW_CASCADE
Command11=ID_WINDOW_TILE_HORZ
Command12=ID_WINDOW_ARRANGE
Command13=ID_APP_ABOUT
CommandCount=13

[MNU:IDR_EVENTS (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_CLOSE
Command3=ID_APP_EXIT
Command4=ID_VIEW_MAP
Command5=ID_VIEW_OBJECTBROWSER
Command6=ID_VIEW_EVENTS
Command7=ID_VIEW_CONTROLPANEL
Command8=ID_VIEW_TOOLBAR
Command9=ID_VIEW_STATUS_BAR
Command10=ID_WINDOW_CASCADE
Command11=ID_WINDOW_TILE_HORZ
Command12=ID_WINDOW_ARRANGE
Command13=ID_APP_ABOUT
CommandCount=13

[MNU:IDR_MAPFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_CLOSE
Command3=ID_APP_EXIT
Command4=ID_VIEW_MAP
Command5=ID_VIEW_OBJECTBROWSER
Command6=ID_VIEW_EVENTS
Command7=ID_VIEW_CONTROLPANEL
Command8=ID_VIEW_TOOLBAR
Command9=ID_VIEW_STATUS_BAR
Command10=ID_WINDOW_CASCADE
Command11=ID_WINDOW_TILE_HORZ
Command12=ID_WINDOW_ARRANGE
Command13=ID_APP_ABOUT
CommandCount=13

[MNU:IDR_OBJECTS (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_NEW
Command2=ID_FILE_CLOSE
Command3=ID_APP_EXIT
Command4=ID_VIEW_MAP
Command5=ID_VIEW_OBJECTBROWSER
Command6=ID_VIEW_EVENTS
Command7=ID_VIEW_CONTROLPANEL
Command8=ID_VIEW_TOOLBAR
Command9=ID_VIEW_STATUS_BAR
Command10=ID_WINDOW_CASCADE
Command11=ID_WINDOW_TILE_HORZ
Command12=ID_WINDOW_ARRANGE
Command13=ID_APP_ABOUT
CommandCount=13

[ACL:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_EDIT_COPY
Command2=ID_FILE_NEW
Command3=ID_EDIT_PASTE
Command4=ID_EDIT_UNDO
Command5=ID_EDIT_CUT
Command6=ID_VIEW_MAP
Command7=ID_VIEW_OBJECTBROWSER
Command8=ID_NEXT_PANE
Command9=ID_PREV_PANE
Command10=ID_VIEW_EVENTS
Command11=ID_VIEW_CONTROLPANEL
Command12=ID_EDIT_COPY
Command13=ID_EDIT_PASTE
Command14=ID_EDIT_CUT
Command15=ID_EDIT_UNDO
CommandCount=15

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[DLG:IDD_LOGIN (English (U.S.))]
Type=1
Class=CLoginDialog
ControlCount=11
Control1=IDC_EDIT_SERVER,edit,1350631552
Control2=IDC_EDIT_LOGIN,edit,1350631552
Control3=IDC_EDIT_PASSWORD,edit,1350631584
Control4=IDOK,button,1342242817
Control5=IDCANCEL,button,1342242816
Control6=IDC_STATIC,static,1342177294
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342177296
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352

[DLG:IDD_PROGRESS (English (U.S.))]
Type=1
Class=CProgressDialog
ControlCount=2
Control1=IDC_STATIC_TITLE,button,1342177287
Control2=IDC_STATIC_TEXT,static,1342308865

[DLG:IDD_OBJECT_PROPERTIES (English (U.S.))]
Type=1
Class=CObjectPropDlg
ControlCount=1
Control1=IDC_LIST_VIEW,SysListView32,1342275613

[DLG:IDD_DUMMY (English (U.S.))]
Type=1
Class=?
ControlCount=3
Control1=IDOK,button,1342242817
Control2=IDCANCEL,button,1342242816
Control3=IDC_TREE_VIEW,SysTreeView32,1350631424

[CLS:CEventEditor]
Type=0
HeaderFile=EventEditor.h
ImplementationFile=EventEditor.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC
LastObject=CEventEditor

[DLG:IDD_EDIT_EVENT]
Type=1
Class=CEditEventDlg
ControlCount=13
Control1=IDC_EDIT_ID,edit,1350633600
Control2=IDC_COMBO_SEVERITY,combobox,1344340227
Control3=IDC_EDIT_NAME,edit,1350631552
Control4=IDC_CHECK_LOG,button,1342242819
Control5=IDC_EDIT_MESSAGE,edit,1350631552
Control6=IDC_EDIT_DESCRIPTION,edit,1352732868
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342308354

[CLS:CEditEventDlg]
Type=0
HeaderFile=EditEventDlg.h
ImplementationFile=EditEventDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CEditEventDlg
VirtualFilter=dWC

[MNU:IDM_VIEW_SPECIFIC]
Type=1
Class=?
Command1=ID_WINDOW_CASCADE
Command2=ID_WINDOW_TILE_VERT
Command3=ID_WINDOW_ARRANGE
Command4=ID_EVENTS_DETAILS
Command5=ID_EVENTS_GOTOSOURCE
CommandCount=5

[ACL:IDA_MDI_DEFAULT]
Type=1
Class=?
Command1=ID_VIEW_REFRESH
CommandCount=1

[CLS:CDebugFrame]
Type=0
HeaderFile=DebugFrame.h
ImplementationFile=DebugFrame.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC

[MNU:IDM_VIEW_SPECIFIC (English (U.S.))]
Type=1
Class=?
Command1=ID_WINDOW_CASCADE
Command2=ID_WINDOW_TILE_VERT
Command3=ID_WINDOW_ARRANGE
Command4=ID_EVENTS_DETAILS
Command5=ID_EVENTS_GOTOSOURCE
Command6=ID_OBJECT_VIEW_SHOWPREVIEWPANE
Command7=ID_OBJECT_VIEW_VIEWASTREE
Command8=ID_OBJECT_VIEW_VIEWASLIST
Command9=ID_OBJECT_FIND
Command10=ID_OBJECT_RENAME
Command11=ID_OBJECT_DELETE
Command12=ID_OBJECT_PROPERTIES
CommandCount=12

[ACL:IDA_MDI_DEFAULT (English (U.S.))]
Type=1
Class=?
Command1=ID_VIEW_REFRESH
CommandCount=1

[DLG:IDD_EDIT_EVENT (English (U.S.))]
Type=1
Class=CEditEventDlg
ControlCount=13
Control1=IDC_EDIT_ID,edit,1350633600
Control2=IDC_COMBO_SEVERITY,combobox,1344340227
Control3=IDC_EDIT_NAME,edit,1350631552
Control4=IDC_CHECK_LOG,button,1342242819
Control5=IDC_EDIT_MESSAGE,edit,1350631552
Control6=IDC_EDIT_DESCRIPTION,edit,1352732868
Control7=IDOK,button,1342242817
Control8=IDCANCEL,button,1342242816
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342308354

[CLS:CObjectPreview]
Type=0
HeaderFile=ObjectPreview.h
ImplementationFile=ObjectPreview.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC

[ACL:IDA_OBJECT_BROWSER]
Type=1
Class=?
Command1=ID_OBJECT_FIND
Command2=ID_OBJECT_VIEW_SHOWPREVIEWPANE
Command3=ID_VIEW_REFRESH
CommandCount=3

[CLS:CToolBox]
Type=0
HeaderFile=ToolBox.h
ImplementationFile=ToolBox.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC

