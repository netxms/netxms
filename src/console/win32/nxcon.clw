; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CDCIPropDlg
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "nxcon.h"
LastPage=0

ClassCount=38
Class1=CConsoleApp
Class3=CMainFrame
Class4=CChildFrame
Class7=CEventBrowser
Class9=CMapView

ResourceCount=44
Resource1=IDD_GROUP_PROPERTIES (English (U.S.))
Resource2=IDD_OBJECT_CAPS
Resource3=IDD_SET_PASSWORD (English (U.S.))
Resource4=IDD_OBJECT_GENERAL
Resource5=IDD_EDIT_EVENT (English (U.S.))
Class2=CChildView
Class5=CAboutDlg
Class6=CControlPanel
Class8=CMapFrame
Class10=CLoginDialog
Resource6=IDD_SELECT_USER
Class11=CProgressDialog
Resource7=IDD_NEW_USER
Class12=CObjectBrowser
Resource8=IDD_SELECT_USER (English (U.S.))
Class13=CObjectPropDlg
Resource9=IDD_REQUEST_PROCESSING
Resource10=IDD_OBJECT_CAPS (English (U.S.))
Resource11=IDD_OBJECT_SECURITY (English (U.S.))
Resource12=IDR_CTRLPANEL (English (U.S.))
Resource13=IDR_EVENTS (English (U.S.))
Resource14=IDR_MAPFRAME (English (U.S.))
Resource15=IDR_OBJECTS (English (U.S.))
Resource16=IDD_DUMMY (English (U.S.))
Class14=CEventEditor
Class15=CEditEventDlg
Resource17=IDA_MDI_DEFAULT (English (U.S.))
Class16=CDebugFrame
Resource18=IDM_CONTEXT (English (U.S.))
Resource19=IDD_OBJECT_PROPERTIES (English (U.S.))
Resource20=IDD_USER_PROPERTIES
Class17=CObjectPreview
Resource21=IDM_VIEW_SPECIFIC (English (U.S.))
Class18=CToolBox
Class19=CObjectInfoBox
Class20=CObjectSearchBox
Resource22=IDD_ABOUTBOX
Class21=CEditBox
Class22=COPGeneral
Class23=CNodePropsGeneral
Resource23=IDD_NEW_USER (English (U.S.))
Class24=CObjectPropCaps
Class25=CObjectPropSheet
Resource24=IDD_OBJECT_NODE_GENERAL
Class26=CRequestProcessingDlg
Resource25=IDD_OBJECT_SECURITY
Resource26=IDD_PROGRESS (English (U.S.))
Resource27=IDD_GROUP_PROPERTIES
Resource28=IDD_USER_PROPERTIES (English (U.S.))
Class27=CObjectPropsGeneral
Resource29=IDD_LOGIN (English (U.S.))
Class28=CObjectPropsSecurity
Resource30=IDD_REQUEST_PROCESSING (English (U.S.))
Resource31=IDD_ABOUTBOX (English (U.S.))
Resource32=IDA_MDI_DEFAULT
Class29=CUserSelectDlg
Resource33=IDD_EDIT_EVENT
Class30=CUserEditor
Resource34=IDR_MAINFRAME (English (U.S.))
Class31=CNewUserDlg
Resource35=IDA_OBJECT_BROWSER
Resource36=IDD_SET_PASSWORD
Class32=CUserPropDlg
Resource37=IDD_OBJECT_NODE_GENERAL (English (U.S.))
Resource38=IDR_MAINFRAME
Class33=CGroupPropDlg
Resource39=IDM_VIEW_SPECIFIC
Resource40=IDD_LOGIN
Resource41=IDM_CONTEXT
Resource42=IDA_OBJECT_BROWSER (English (U.S.))
Class34=CPasswordChangeDlg
Class35=CNodeSummary
Class36=CNetSummaryFrame
Class37=CDataCollectionEditor
Resource43=IDD_OBJECT_GENERAL (English (U.S.))
Class38=CDCIPropDlg
Resource44=IDD_DCI_PROPERTIES (English (U.S.))

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
Command1=ID_FILE_SETTINGS
Command2=ID_FILE_EXPORT
Command3=ID_FILE_PAGESETUP
Command4=ID_FILE_PRINT
Command5=ID_APP_EXIT
Command6=ID_VIEW_MAP
Command7=ID_VIEW_OBJECTBROWSER
Command8=ID_VIEW_EPP
Command9=ID_VIEW_NETWORKSUMMARY
Command10=ID_VIEW_EVENTS
Command11=ID_VIEW_CONTROLPANEL
Command12=ID_VIEW_DEBUG
Command13=ID_VIEW_TOOLBAR
Command14=ID_VIEW_STATUS_BAR
Command15=ID_VIEW_REFRESH
Command16=ID_CONTROLPANEL_EVENTS
Command17=ID_CONTROLPANEL_USERS
Command18=ID_APP_ABOUT
CommandCount=18

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
Command7=ID_VIEW_OBJECTBROWSER
Command8=ID_NEXT_PANE
Command9=ID_PREV_PANE
Command10=ID_VIEW_NETWORKSUMMARY
Command11=ID_VIEW_EVENTS
Command12=ID_VIEW_CONTROLPANEL
Command13=ID_EDIT_COPY
Command14=ID_EDIT_PASTE
Command15=ID_EDIT_CUT
Command16=ID_EDIT_UNDO
CommandCount=16

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
Command9=ID_VIEW_NETWORKSUMMARY
Command10=ID_VIEW_EVENTS
Command11=ID_VIEW_CONTROLPANEL
Command12=ID_VIEW_DEBUG
Command13=ID_VIEW_TOOLBAR
Command14=ID_VIEW_STATUS_BAR
Command15=ID_VIEW_REFRESH
Command16=ID_CONTROLPANEL_EVENTS
Command17=ID_CONTROLPANEL_USERS
Command18=ID_APP_ABOUT
CommandCount=18

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
Command10=ID_VIEW_NETWORKSUMMARY
Command11=ID_VIEW_EVENTS
Command12=ID_VIEW_CONTROLPANEL
Command13=ID_EDIT_COPY
Command14=ID_EDIT_PASTE
Command15=ID_EDIT_CUT
Command16=ID_EDIT_UNDO
CommandCount=16

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
Command6=ID_OBJECT_VIEW_SHOWPREVIEWPANE
Command7=ID_OBJECT_VIEW_SELECTION
Command8=ID_OBJECT_VIEW_VIEWASTREE
Command9=ID_OBJECT_VIEW_VIEWASLIST
Command10=ID_OBJECT_FIND
Command11=ID_OBJECT_RENAME
Command12=ID_OBJECT_DELETE
Command13=ID_OBJECT_MANAGE
Command14=ID_OBJECT_UNMANAGE
Command15=ID_OBJECT_DATACOLLECTION
Command16=ID_OBJECT_PROPERTIES
Command17=ID_USER_CREATE_USER
Command18=ID_USER_CREATE_GROUP
Command19=ID_USER_DELETE
Command20=ID_USER_SETPASSWORD
Command21=ID_USER_PROPERTIES
CommandCount=21

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
Command7=ID_OBJECT_VIEW_SELECTION
Command8=ID_OBJECT_VIEW_VIEWASTREE
Command9=ID_OBJECT_VIEW_VIEWASLIST
Command10=ID_OBJECT_FIND
Command11=ID_OBJECT_RENAME
Command12=ID_OBJECT_DELETE
Command13=ID_OBJECT_MANAGE
Command14=ID_OBJECT_UNMANAGE
Command15=ID_OBJECT_DATACOLLECTION
Command16=ID_OBJECT_PROPERTIES
Command17=ID_USER_CREATE_USER
Command18=ID_USER_CREATE_GROUP
Command19=ID_USER_DELETE
Command20=ID_USER_SETPASSWORD
Command21=ID_USER_PROPERTIES
Command22=ID_ITEM_NEW
Command23=ID_ITEM_EDIT
Command24=ID_ITEM_DELETE
Command25=ID_ITEM_ACTIVATE
Command26=ID_ITEM_DISABLE
CommandCount=26

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
Command2=ID_OBJECT_VIEW_SELECTION
Command3=ID_OBJECT_VIEW_VIEWASLIST
Command4=ID_OBJECT_VIEW_SHOWPREVIEWPANE
Command5=ID_OBJECT_VIEW_VIEWASTREE
Command6=ID_VIEW_REFRESH
CommandCount=6

[CLS:CToolBox]
Type=0
HeaderFile=ToolBox.h
ImplementationFile=ToolBox.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC

[CLS:CObjectInfoBox]
Type=0
HeaderFile=ObjectInfoBox.h
ImplementationFile=ObjectInfoBox.cpp
BaseClass=CToolBox
Filter=W
LastObject=CObjectInfoBox

[CLS:CObjectSearchBox]
Type=0
HeaderFile=ObjectSearchBox.h
ImplementationFile=ObjectSearchBox.cpp
BaseClass=CToolBox
Filter=W

[ACL:IDA_OBJECT_BROWSER (English (U.S.))]
Type=1
Class=?
Command1=ID_OBJECT_FIND
Command2=ID_OBJECT_VIEW_SELECTION
Command3=ID_OBJECT_VIEW_VIEWASLIST
Command4=ID_OBJECT_VIEW_SHOWPREVIEWPANE
Command5=ID_OBJECT_VIEW_VIEWASTREE
Command6=ID_VIEW_REFRESH
CommandCount=6

[CLS:CEditBox]
Type=0
HeaderFile=EditBox.h
ImplementationFile=EditBox.cpp
BaseClass=CEdit
Filter=W
VirtualFilter=WC
LastObject=CEditBox

[CLS:COPGeneral]
Type=0
HeaderFile=OPGeneral.h
ImplementationFile=OPGeneral.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=COPGeneral

[DLG:IDD_OBJECT_NODE_GENERAL]
Type=1
Class=CNodePropsGeneral
ControlCount=22
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_ID,edit,1350633600
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_EDIT_NAME,edit,1350631552
Control6=IDC_EDIT_PRIMARY_IP,edit,1350633600
Control7=IDC_SELECT_IP,button,1342242816
Control8=IDC_STATIC,static,1342308352
Control9=IDC_EDIT_OID,edit,1350633600
Control10=IDC_STATIC,static,1342308352
Control11=IDC_COMBO_AUTH,combobox,1344340227
Control12=IDC_STATIC,static,1342308352
Control13=IDC_EDIT_SECRET,edit,1350631552
Control14=IDC_STATIC,static,1342308352
Control15=IDC_EDIT_PORT,edit,1350639744
Control16=IDC_STATIC,static,1342308352
Control17=IDC_COMBO_SNMP_VERSION,combobox,1344340227
Control18=IDC_STATIC,static,1342308352
Control19=IDC_EDIT_COMMUNITY,edit,1350631552
Control20=IDC_STATIC,button,1342177287
Control21=IDC_STATIC,button,1342177287
Control22=IDC_STATIC,button,1342177287

[CLS:CNodePropsGeneral]
Type=0
HeaderFile=NodePropsGeneral.h
ImplementationFile=NodePropsGeneral.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CNodePropsGeneral

[DLG:IDD_OBJECT_CAPS]
Type=1
Class=CObjectPropCaps
ControlCount=1
Control1=IDC_LIST_CAPS,SysListView32,1342275613

[CLS:CObjectPropCaps]
Type=0
HeaderFile=ObjectPropCaps.h
ImplementationFile=ObjectPropCaps.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC

[CLS:CObjectPropSheet]
Type=0
HeaderFile=ObjectPropSheet.h
ImplementationFile=ObjectPropSheet.cpp
BaseClass=CPropertySheet
Filter=W
LastObject=CObjectPropSheet
VirtualFilter=hWC

[DLG:IDD_REQUEST_PROCESSING]
Type=1
Class=CRequestProcessingDlg
ControlCount=2
Control1=IDC_STATIC,static,1342177283
Control2=IDC_INFO_TEXT,static,1342308352

[CLS:CRequestProcessingDlg]
Type=0
HeaderFile=RequestProcessingDlg.h
ImplementationFile=RequestProcessingDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CRequestProcessingDlg

[DLG:IDD_OBJECT_NODE_GENERAL (English (U.S.))]
Type=1
Class=CNodePropsGeneral
ControlCount=22
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_ID,edit,1350633600
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342308352
Control5=IDC_EDIT_NAME,edit,1350631552
Control6=IDC_EDIT_PRIMARY_IP,edit,1350633600
Control7=IDC_SELECT_IP,button,1342242816
Control8=IDC_STATIC,static,1342308352
Control9=IDC_EDIT_OID,edit,1350633600
Control10=IDC_STATIC,static,1342308352
Control11=IDC_COMBO_AUTH,combobox,1344340227
Control12=IDC_STATIC,static,1342308352
Control13=IDC_EDIT_SECRET,edit,1350631552
Control14=IDC_STATIC,static,1342308352
Control15=IDC_EDIT_PORT,edit,1350639744
Control16=IDC_STATIC,static,1342308352
Control17=IDC_COMBO_SNMP_VERSION,combobox,1344340227
Control18=IDC_STATIC,static,1342308352
Control19=IDC_EDIT_COMMUNITY,edit,1350631552
Control20=IDC_STATIC,button,1342177287
Control21=IDC_STATIC,button,1342177287
Control22=IDC_STATIC,button,1342177287

[DLG:IDD_OBJECT_CAPS (English (U.S.))]
Type=1
Class=CObjectPropCaps
ControlCount=1
Control1=IDC_LIST_CAPS,SysListView32,1342275613

[DLG:IDD_REQUEST_PROCESSING (English (U.S.))]
Type=1
Class=CRequestProcessingDlg
ControlCount=2
Control1=IDC_STATIC,static,1342177283
Control2=IDC_INFO_TEXT,static,1342308352

[DLG:IDD_OBJECT_GENERAL (English (U.S.))]
Type=1
Class=CObjectPropsGeneral
ControlCount=6
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_ID,edit,1350633600
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_CLASS,edit,1350633600
Control5=IDC_STATIC,static,1342308352
Control6=IDC_EDIT_NAME,edit,1350631552

[CLS:CObjectPropsGeneral]
Type=0
HeaderFile=ObjectPropsGeneral.h
ImplementationFile=ObjectPropsGeneral.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CObjectPropsGeneral
VirtualFilter=idWC

[DLG:IDD_OBJECT_SECURITY (English (U.S.))]
Type=1
Class=CObjectPropsSecurity
ControlCount=13
Control1=IDC_LIST_USERS,SysListView32,1342259485
Control2=IDC_CHECK_READ,button,1342242819
Control3=IDC_CHECK_MODIFY,button,1342242819
Control4=IDC_CHECK_CREATE,button,1342242819
Control5=IDC_CHECK_DELETE,button,1342242819
Control6=IDC_CHECK_MOVE,button,1342242819
Control7=IDC_CHECK_ACCESS,button,1342242819
Control8=IDC_ADD_USER,button,1342242816
Control9=IDC_DELETE_USER,button,1342242816
Control10=IDC_CHECK_INHERIT_RIGHTS,button,1342242819
Control11=IDC_STATIC,button,1342177287
Control12=IDC_STATIC,button,1342177287
Control13=IDC_STATIC,button,1342177287

[CLS:CObjectPropsSecurity]
Type=0
HeaderFile=ObjectPropsSecurity.h
ImplementationFile=ObjectPropsSecurity.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CObjectPropsSecurity

[DLG:IDD_OBJECT_GENERAL]
Type=1
Class=CObjectPropsGeneral
ControlCount=6
Control1=IDC_STATIC,static,1342308352
Control2=IDC_EDIT_ID,edit,1350633600
Control3=IDC_STATIC,static,1342308352
Control4=IDC_EDIT_CLASS,edit,1350633600
Control5=IDC_STATIC,static,1342308352
Control6=IDC_EDIT_NAME,edit,1350631552

[DLG:IDD_OBJECT_SECURITY]
Type=1
Class=CObjectPropsSecurity
ControlCount=13
Control1=IDC_LIST_USERS,SysListView32,1342259485
Control2=IDC_CHECK_READ,button,1342242819
Control3=IDC_CHECK_MODIFY,button,1342242819
Control4=IDC_CHECK_CREATE,button,1342242819
Control5=IDC_CHECK_DELETE,button,1342242819
Control6=IDC_CHECK_MOVE,button,1342242819
Control7=IDC_CHECK_ACCESS,button,1342242819
Control8=IDC_ADD_USER,button,1342242816
Control9=IDC_DELETE_USER,button,1342242816
Control10=IDC_CHECK_INHERIT_RIGHTS,button,1342242819
Control11=IDC_STATIC,button,1342177287
Control12=IDC_STATIC,button,1342177287
Control13=IDC_STATIC,button,1342177287

[DLG:IDD_SELECT_USER]
Type=1
Class=CUserSelectDlg
ControlCount=4
Control1=IDC_LIST_USERS,SysListView32,1342291997
Control2=IDOK,button,1342242817
Control3=IDCANCEL,button,1342242816
Control4=IDC_STATIC,static,1342308352

[CLS:CUserSelectDlg]
Type=0
HeaderFile=UserSelectDlg.h
ImplementationFile=UserSelectDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CUserSelectDlg

[DLG:IDD_SELECT_USER (English (U.S.))]
Type=1
Class=CUserSelectDlg
ControlCount=4
Control1=IDC_LIST_USERS,SysListView32,1342291997
Control2=IDOK,button,1342242817
Control3=IDCANCEL,button,1342242816
Control4=IDC_STATIC,static,1342308352

[CLS:CUserEditor]
Type=0
HeaderFile=UserEditor.h
ImplementationFile=UserEditor.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC
LastObject=CUserEditor

[DLG:IDD_NEW_USER (English (U.S.))]
Type=1
Class=CNewUserDlg
ControlCount=5
Control1=IDC_EDIT_NAME,edit,1350631552
Control2=IDC_CHECK_PROPERTIES,button,1342242819
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816
Control5=IDC_STATIC_HEADER,static,1342308352

[CLS:CNewUserDlg]
Type=0
HeaderFile=NewUserDlg.h
ImplementationFile=NewUserDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CNewUserDlg
VirtualFilter=dWC

[DLG:IDD_NEW_USER]
Type=1
Class=CNewUserDlg
ControlCount=5
Control1=IDC_EDIT_NAME,edit,1350631552
Control2=IDC_CHECK_PROPERTIES,button,1342242819
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816
Control5=IDC_STATIC_HEADER,static,1342308352

[DLG:IDD_USER_PROPERTIES (English (U.S.))]
Type=1
Class=CUserPropDlg
ControlCount=18
Control1=IDC_EDIT_LOGIN,edit,1350631552
Control2=IDC_EDIT_NAME,edit,1350631552
Control3=IDC_EDIT_DESCRIPTION,edit,1350631552
Control4=IDC_CHECK_DISABLED,button,1342242819
Control5=IDC_CHECK_PASSWORD,button,1342242819
Control6=IDC_CHECK_MANAGE_USERS,button,1342242819
Control7=IDC_CHECK_VIEW_CONFIG,button,1342242819
Control8=IDC_CHECK_EDIT_CONFIG,button,1342242819
Control9=IDC_CHECK_DROP_CONN,button,1342242819
Control10=IDC_CHECK_VIEW_EVENTDB,button,1342242819
Control11=IDC_CHECK_EDIT_EVENTDB,button,1342242819
Control12=IDOK,button,1342242817
Control13=IDCANCEL,button,1342242816
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,static,1342308352
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,button,1342177287
Control18=IDC_STATIC,button,1342177287

[CLS:CUserPropDlg]
Type=0
HeaderFile=UserPropDlg.h
ImplementationFile=UserPropDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CUserPropDlg
VirtualFilter=dWC

[DLG:IDD_USER_PROPERTIES]
Type=1
Class=CUserPropDlg
ControlCount=18
Control1=IDC_EDIT_LOGIN,edit,1350631552
Control2=IDC_EDIT_NAME,edit,1350631552
Control3=IDC_EDIT_DESCRIPTION,edit,1350631552
Control4=IDC_CHECK_DISABLED,button,1342242819
Control5=IDC_CHECK_PASSWORD,button,1342242819
Control6=IDC_CHECK_MANAGE_USERS,button,1342242819
Control7=IDC_CHECK_VIEW_CONFIG,button,1342242819
Control8=IDC_CHECK_EDIT_CONFIG,button,1342242819
Control9=IDC_CHECK_DROP_CONN,button,1342242819
Control10=IDC_CHECK_VIEW_EVENTDB,button,1342242819
Control11=IDC_CHECK_EDIT_EVENTDB,button,1342242819
Control12=IDOK,button,1342242817
Control13=IDCANCEL,button,1342242816
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,static,1342308352
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,button,1342177287
Control18=IDC_STATIC,button,1342177287

[DLG:IDD_GROUP_PROPERTIES]
Type=1
Class=CGroupPropDlg
ControlCount=19
Control1=IDC_EDIT_NAME,edit,1350631552
Control2=IDC_EDIT_DESCRIPTION,edit,1350631552
Control3=IDC_CHECK_DISABLED,button,1342242819
Control4=IDC_CHECK_MANAGE_USERS,button,1342242819
Control5=IDC_CHECK_VIEW_CONFIG,button,1342242819
Control6=IDC_CHECK_EDIT_CONFIG,button,1342242819
Control7=IDC_CHECK_DROP_CONN,button,1342242819
Control8=IDC_CHECK_VIEW_EVENTDB,button,1342242819
Control9=IDC_CHECK_EDIT_EVENTDB,button,1342242819
Control10=IDC_LIST_MEMBERS,SysListView32,1342291997
Control11=IDC_BUTTON_ADD,button,1342242816
Control12=IDC_BUTTON_DELETE,button,1342242816
Control13=IDOK,button,1342242817
Control14=IDCANCEL,button,1342242816
Control15=IDC_STATIC,static,1342308352
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,button,1342177287
Control18=IDC_STATIC,button,1342177287
Control19=IDC_STATIC,button,1342177287

[CLS:CGroupPropDlg]
Type=0
HeaderFile=GroupPropDlg.h
ImplementationFile=GroupPropDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CGroupPropDlg

[DLG:IDD_GROUP_PROPERTIES (English (U.S.))]
Type=1
Class=CGroupPropDlg
ControlCount=19
Control1=IDC_EDIT_NAME,edit,1350631552
Control2=IDC_EDIT_DESCRIPTION,edit,1350631552
Control3=IDC_CHECK_DISABLED,button,1342242819
Control4=IDC_CHECK_MANAGE_USERS,button,1342242819
Control5=IDC_CHECK_VIEW_CONFIG,button,1342242819
Control6=IDC_CHECK_EDIT_CONFIG,button,1342242819
Control7=IDC_CHECK_DROP_CONN,button,1342242819
Control8=IDC_CHECK_VIEW_EVENTDB,button,1342242819
Control9=IDC_CHECK_EDIT_EVENTDB,button,1342242819
Control10=IDC_LIST_MEMBERS,SysListView32,1342291997
Control11=IDC_BUTTON_ADD,button,1342242816
Control12=IDC_BUTTON_DELETE,button,1342242816
Control13=IDOK,button,1342242817
Control14=IDCANCEL,button,1342242816
Control15=IDC_STATIC,static,1342308352
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,button,1342177287
Control18=IDC_STATIC,button,1342177287
Control19=IDC_STATIC,button,1342177287

[MNU:IDM_CONTEXT]
Type=1
Class=?
Command1=ID_USER_SETPASSWORD
Command2=ID_USER_CREATE_USER
Command3=ID_USER_CREATE_GROUP
Command4=ID_USER_RENAME
Command5=ID_USER_DELETE
Command6=ID_USER_PROPERTIES
Command7=ID_OBJECT_RENAME
Command8=ID_OBJECT_DELETE
Command9=ID_OBJECT_MANAGE
Command10=ID_OBJECT_UNMANAGE
Command11=ID_OBJECT_DATACOLLECTION
Command12=ID_OBJECT_PROPERTIES
CommandCount=12

[MNU:IDM_CONTEXT (English (U.S.))]
Type=1
Class=?
Command1=ID_USER_SETPASSWORD
Command2=ID_USER_CREATE_USER
Command3=ID_USER_CREATE_GROUP
Command4=ID_USER_RENAME
Command5=ID_USER_DELETE
Command6=ID_USER_PROPERTIES
Command7=ID_OBJECT_RENAME
Command8=ID_OBJECT_DELETE
Command9=ID_OBJECT_MANAGE
Command10=ID_OBJECT_UNMANAGE
Command11=ID_OBJECT_DATACOLLECTION
Command12=ID_OBJECT_PROPERTIES
Command13=ID_ITEM_NEW
Command14=ID_ITEM_EDIT
Command15=ID_ITEM_DELETE
Command16=ID_ITEM_ACTIVATE
Command17=ID_ITEM_DISABLE
CommandCount=17

[DLG:IDD_SET_PASSWORD (English (U.S.))]
Type=1
Class=CPasswordChangeDlg
ControlCount=7
Control1=IDC_EDIT_PASSWD1,edit,1342242976
Control2=IDC_EDIT_PASSWD2,edit,1342242976
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342177283

[CLS:CPasswordChangeDlg]
Type=0
HeaderFile=PasswordChangeDlg.h
ImplementationFile=PasswordChangeDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CPasswordChangeDlg

[DLG:IDD_SET_PASSWORD]
Type=1
Class=CPasswordChangeDlg
ControlCount=7
Control1=IDC_EDIT_PASSWD1,edit,1342242976
Control2=IDC_EDIT_PASSWD2,edit,1342242976
Control3=IDOK,button,1342242817
Control4=IDCANCEL,button,1342242816
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342177283

[CLS:CNodeSummary]
Type=0
HeaderFile=NodeSummary.h
ImplementationFile=NodeSummary.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC

[CLS:CNetSummaryFrame]
Type=0
HeaderFile=NetSummaryFrame.h
ImplementationFile=NetSummaryFrame.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC

[CLS:CDataCollectionEditor]
Type=0
HeaderFile=DataCollectionEditor.h
ImplementationFile=DataCollectionEditor.cpp
BaseClass=CMDIChildWnd
Filter=M
VirtualFilter=mfWC
LastObject=CDataCollectionEditor

[DLG:IDD_DCI_PROPERTIES (English (U.S.))]
Type=1
Class=CDCIPropDlg
ControlCount=21
Control1=IDC_EDIT_NAME,edit,1350631552
Control2=IDC_BUTTON_SELECT,button,1342242816
Control3=IDC_COMBO_ORIGIN,combobox,1344339971
Control4=IDC_COMBO_DT,combobox,1344339971
Control5=IDC_EDIT_INTERVAL,edit,1350639744
Control6=IDC_EDIT_RETENTION,edit,1350639744
Control7=IDC_RADIO_ACTIVE,button,1342373897
Control8=IDC_RADIO_DISABLED,button,1342242825
Control9=IDC_RADIO6,button,1342242825
Control10=IDOK,button,1342373889
Control11=IDCANCEL,button,1342242816
Control12=IDC_STATIC,button,1342177287
Control13=IDC_STATIC,button,1342177287
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,static,1342308352
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC,button,1342177287
Control19=IDC_STATIC,static,1342308352
Control20=IDC_STATIC,static,1342308352
Control21=IDC_STATIC,static,1342308352

[CLS:CDCIPropDlg]
Type=0
HeaderFile=DCIPropDlg.h
ImplementationFile=DCIPropDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CDCIPropDlg
VirtualFilter=dWC

