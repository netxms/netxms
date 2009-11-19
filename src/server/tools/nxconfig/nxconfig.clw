; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CSrvDepsPage
LastTemplate=CPropertyPage
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "nxconfig.h"
LastPage=0

ClassCount=17
Class1=CNxconfigApp
Class3=CMainFrame
Class4=CAboutDlg

ResourceCount=27
Resource1=IDD_SMTP (English (U.S.))
Resource2=IDR_MAINFRAME (English (U.S.))
Class2=CChildView
Resource3=IDD_INTRO (English (U.S.))
Resource4=IDD_POLLING (English (U.S.))
Class5=CIntroPage
Class6=CConfigWizard
Resource5=IDD_LOG_FILE (English (U.S.))
Class7=CDBSelectPage
Resource6=IDD_PROCESSING (English (U.S.))
Class8=CFinishPage
Resource7=IDD_SUMMARY (English (U.S.))
Class9=CODBCPage
Resource8=IDD_ABOUTBOX (English (U.S.))
Class10=CPollCfgPage
Resource9=IDR_MAINFRAME
Class11=CSMTPPage
Resource10=IDD_SELECT_DB (English (U.S.))
Class12=CSummaryPage
Resource11=IDD_CFG_FILE (English (U.S.))
Class13=CProcessingPage
Class14=CConfigFilePage
Class15=CLoggingPage
Resource12=IDD_SERVICE (English (U.S.))
Class16=CWinSrvPage
Resource13=IDD_ODBC (English (U.S.))
Resource14=IDD_CFG_FILE
Resource15=IDD_SERVICE
Resource16=IDD_FINISH
Resource17=IDD_POLLING
Resource18=IDD_PROCESSING
Resource19=IDD_SRV_DEPS
Resource20=IDD_INTRO
Resource21=IDD_ODBC
Resource22=IDD_SELECT_DB
Resource23=IDD_SMTP
Resource24=IDD_SUMMARY
Resource25=IDD_LOG_FILE
Class17=CSrvDepsPage
Resource26=IDD_FINISH (English (U.S.))
Resource27=IDD_SRV_DEPS (English (U.S.))

[CLS:CNxconfigApp]
Type=0
HeaderFile=nxconfig.h
ImplementationFile=nxconfig.cpp
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




[CLS:CAboutDlg]
Type=0
HeaderFile=nxconfig.cpp
ImplementationFile=nxconfig.cpp
Filter=D
LastObject=CAboutDlg

[MNU:IDR_MAINFRAME]
Type=1
Class=CMainFrame
Command1=ID_FILE_CFG_WIZARD
Command2=ID_APP_EXIT
CommandCount=2

[MNU:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
Command1=ID_FILE_CFG_WIZARD
Command2=ID_APP_EXIT
CommandCount=2

[ACL:IDR_MAINFRAME (English (U.S.))]
Type=1
Class=?
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

[DLG:IDD_ABOUTBOX (English (U.S.))]
Type=1
Class=CAboutDlg
ControlCount=4
Control1=IDC_STATIC,static,1342177283
Control2=IDC_STATIC,static,1342308480
Control3=IDC_STATIC,static,1342308352
Control4=IDOK,button,1342373889

[CLS:CIntroPage]
Type=0
HeaderFile=IntroPage.h
ImplementationFile=IntroPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CIntroPage
VirtualFilter=idWC

[CLS:CConfigWizard]
Type=0
HeaderFile=ConfigWizard.h
ImplementationFile=ConfigWizard.cpp
BaseClass=CPropertySheet
Filter=W
LastObject=CConfigWizard

[DLG:IDD_INTRO (English (U.S.))]
Type=1
Class=CIntroPage
ControlCount=2
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342177806

[CLS:CDBSelectPage]
Type=0
HeaderFile=DBSelectPage.h
ImplementationFile=DBSelectPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CDBSelectPage
VirtualFilter=idWC

[CLS:CFinishPage]
Type=0
HeaderFile=FinishPage.h
ImplementationFile=FinishPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CFinishPage
VirtualFilter=idWC

[DLG:IDD_ODBC (English (U.S.))]
Type=1
Class=CODBCPage
ControlCount=5
Control1=IDC_BUTTON_ODBC,button,1342242816
Control2=IDC_STATIC,static,1342177806
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342177283
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_SELECT_DB (English (U.S.))]
Type=1
Class=CDBSelectPage
ControlCount=23
Control1=IDC_COMBO_DBENGINE,combobox,1344340227
Control2=IDC_COMBO_DBDRV,combobox,1344340227
Control3=IDC_EDIT_SERVER,edit,1350631552
Control4=IDC_RADIO_NEWDB,button,1342373897
Control5=IDC_RADIO_EXISTINGDB,button,1342177289
Control6=IDC_CHECK_INITDB,button,1342242819
Control7=IDC_EDIT_DBA_LOGIN,edit,1350631552
Control8=IDC_EDIT_DBA_PASSWORD,edit,1350631584
Control9=IDC_EDIT_DB_NAME,edit,1350631552
Control10=IDC_EDIT_DB_LOGIN,edit,1350631552
Control11=IDC_EDIT_DB_PASSWORD,edit,1350631584
Control12=IDC_STATIC,static,1342177806
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC_SERVER,static,1342308352
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC_DBNAME,static,1342308352
Control19=IDC_STATIC,static,1342308352
Control20=IDC_STATIC,static,1342308352
Control21=IDC_STATIC,static,1342177296
Control22=IDC_STATIC,static,1342177296
Control23=IDC_STATIC,static,1342177296

[DLG:IDD_FINISH (English (U.S.))]
Type=1
Class=CFinishPage
ControlCount=2
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC,static,1342308352

[CLS:CODBCPage]
Type=0
HeaderFile=ODBCPage.h
ImplementationFile=ODBCPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC

[DLG:IDD_POLLING (English (U.S.))]
Type=1
Class=CPollCfgPage
ControlCount=18
Control1=IDC_CHECK_RUN_DISCOVERY,button,1342242819
Control2=IDC_EDIT_INT_DP,edit,1350639744
Control3=IDC_EDIT_NUM_SP,edit,1350631552
Control4=IDC_EDIT_INT_SP,edit,1350639744
Control5=IDC_EDIT_NUM_CP,edit,1350631552
Control6=IDC_EDIT_INT_CP,edit,1350639744
Control7=IDC_STATIC,static,1342177806
Control8=IDC_STATIC,button,1342177287
Control9=IDC_STATIC_DI,static,1342308352
Control10=IDC_STATIC_SEC,static,1342308352
Control11=IDC_STATIC,button,1342177287
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,button,1342177287
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC,static,1342308352

[CLS:CPollCfgPage]
Type=0
HeaderFile=PollCfgPage.h
ImplementationFile=PollCfgPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CPollCfgPage

[DLG:IDD_SMTP (English (U.S.))]
Type=1
Class=CSMTPPage
ControlCount=13
Control1=IDC_EDIT_SERVER,edit,1350631552
Control2=IDC_EDIT_EMAIL,edit,1350631552
Control3=IDC_STATIC,static,1342177806
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_COMBO_SMSDRV,combobox,1344340227
Control8=IDC_STATIC_PORT,static,1342308352
Control9=IDC_COMBO_PORT,combobox,1344340227
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342177296
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342177296

[CLS:CSMTPPage]
Type=0
HeaderFile=SMTPPage.h
ImplementationFile=SMTPPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CSMTPPage

[DLG:IDD_SUMMARY (English (U.S.))]
Type=1
Class=CSummaryPage
ControlCount=3
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC,static,1342308352
Control3=IDC_EDIT_SUMMARY,edit,1352665284

[CLS:CSummaryPage]
Type=0
HeaderFile=SummaryPage.h
ImplementationFile=SummaryPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CSummaryPage
VirtualFilter=idWC

[DLG:IDD_PROCESSING (English (U.S.))]
Type=1
Class=CProcessingPage
ControlCount=3
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC_STATUS,static,1342308352
Control3=IDC_LIST_STATUS,SysListView32,1342291969

[CLS:CProcessingPage]
Type=0
HeaderFile=ProcessingPage.h
ImplementationFile=ProcessingPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CProcessingPage

[DLG:IDD_CFG_FILE (English (U.S.))]
Type=1
Class=CConfigFilePage
ControlCount=5
Control1=IDC_EDIT_FILE,edit,1350631552
Control2=IDC_BUTTON_BROWSE,button,1342242816
Control3=IDC_STATIC,static,1342177806
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352

[CLS:CConfigFilePage]
Type=0
HeaderFile=ConfigFilePage.h
ImplementationFile=ConfigFilePage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=CConfigFilePage
VirtualFilter=idWC

[DLG:IDD_LOG_FILE (English (U.S.))]
Type=1
Class=CLoggingPage
ControlCount=6
Control1=IDC_RADIO_SYSLOG,button,1342373897
Control2=IDC_RADIO_FILE,button,1342177289
Control3=IDC_EDIT_FILE,edit,1350631552
Control4=IDC_BUTTON_BROWSE,button,1342242816
Control5=IDC_STATIC,static,1342177806
Control6=IDC_STATIC,static,1342308352

[CLS:CLoggingPage]
Type=0
HeaderFile=LoggingPage.h
ImplementationFile=LoggingPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC

[DLG:IDD_SERVICE (English (U.S.))]
Type=1
Class=CWinSrvPage
ControlCount=11
Control1=IDC_RADIO_SYSTEM,button,1342373897
Control2=IDC_RADIO_USER,button,1342242825
Control3=IDC_EDIT_LOGIN,edit,1350631552
Control4=IDC_EDIT_PASSWD1,edit,1350631584
Control5=IDC_EDIT_PASSWD2,edit,1350631584
Control6=IDC_STATIC,static,1342177806
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_ICON_WARNING,static,1342177283
Control11=IDC_STATIC_WARNING,static,1342308352

[CLS:CWinSrvPage]
Type=0
HeaderFile=WinSrvPage.h
ImplementationFile=WinSrvPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC

[DLG:IDD_LOG_FILE]
Type=1
Class=CLoggingPage
ControlCount=6
Control1=IDC_RADIO_SYSLOG,button,1342373897
Control2=IDC_RADIO_FILE,button,1342177289
Control3=IDC_EDIT_FILE,edit,1350631552
Control4=IDC_BUTTON_BROWSE,button,1342242816
Control5=IDC_STATIC,static,1342177806
Control6=IDC_STATIC,static,1342308352

[DLG:IDD_SERVICE]
Type=1
Class=CWinSrvPage
ControlCount=11
Control1=IDC_RADIO_SYSTEM,button,1342373897
Control2=IDC_RADIO_USER,button,1342242825
Control3=IDC_EDIT_LOGIN,edit,1350631552
Control4=IDC_EDIT_PASSWD1,edit,1350631584
Control5=IDC_EDIT_PASSWD2,edit,1350631584
Control6=IDC_STATIC,static,1342177806
Control7=IDC_STATIC,static,1342308352
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_ICON_WARNING,static,1342177283
Control11=IDC_STATIC_WARNING,static,1342308352

[DLG:IDD_ODBC]
Type=1
Class=CODBCPage
ControlCount=5
Control1=IDC_BUTTON_ODBC,button,1342242816
Control2=IDC_STATIC,static,1342177806
Control3=IDC_STATIC,static,1342308352
Control4=IDC_STATIC,static,1342177283
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_FINISH]
Type=1
Class=CFinishPage
ControlCount=2
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC,static,1342308352

[DLG:IDD_CFG_FILE]
Type=1
Class=CConfigFilePage
ControlCount=5
Control1=IDC_EDIT_FILE,edit,1350631552
Control2=IDC_BUTTON_BROWSE,button,1342242816
Control3=IDC_STATIC,static,1342177806
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352

[DLG:IDD_INTRO]
Type=1
Class=CIntroPage
ControlCount=2
Control1=IDC_STATIC,static,1342308352
Control2=IDC_STATIC,static,1342177806

[DLG:IDD_SELECT_DB]
Type=1
Class=CDBSelectPage
ControlCount=23
Control1=IDC_COMBO_DBENGINE,combobox,1344340227
Control2=IDC_COMBO_DBDRV,combobox,1344340227
Control3=IDC_EDIT_SERVER,edit,1350631552
Control4=IDC_RADIO_NEWDB,button,1342373897
Control5=IDC_RADIO_EXISTINGDB,button,1342177289
Control6=IDC_CHECK_INITDB,button,1342242819
Control7=IDC_EDIT_DBA_LOGIN,edit,1350631552
Control8=IDC_EDIT_DBA_PASSWORD,edit,1350631584
Control9=IDC_EDIT_DB_NAME,edit,1350631552
Control10=IDC_EDIT_DB_LOGIN,edit,1350631552
Control11=IDC_EDIT_DB_PASSWORD,edit,1350631584
Control12=IDC_STATIC,static,1342177806
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC_SERVER,static,1342308352
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC_DBNAME,static,1342308352
Control19=IDC_STATIC,static,1342308352
Control20=IDC_STATIC,static,1342308352
Control21=IDC_STATIC,static,1342177296
Control22=IDC_STATIC,static,1342177296
Control23=IDC_STATIC,static,1342177296

[DLG:IDD_POLLING]
Type=1
Class=CPollCfgPage
ControlCount=18
Control1=IDC_CHECK_RUN_DISCOVERY,button,1342242819
Control2=IDC_EDIT_INT_DP,edit,1350639744
Control3=IDC_EDIT_NUM_SP,edit,1350631552
Control4=IDC_EDIT_INT_SP,edit,1350639744
Control5=IDC_EDIT_NUM_CP,edit,1350631552
Control6=IDC_EDIT_INT_CP,edit,1350639744
Control7=IDC_STATIC,static,1342177806
Control8=IDC_STATIC,button,1342177287
Control9=IDC_STATIC_DI,static,1342308352
Control10=IDC_STATIC_SEC,static,1342308352
Control11=IDC_STATIC,button,1342177287
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,button,1342177287
Control16=IDC_STATIC,static,1342308352
Control17=IDC_STATIC,static,1342308352
Control18=IDC_STATIC,static,1342308352

[DLG:IDD_SMTP]
Type=1
Class=CSMTPPage
ControlCount=13
Control1=IDC_EDIT_SERVER,edit,1350631552
Control2=IDC_EDIT_EMAIL,edit,1350631552
Control3=IDC_STATIC,static,1342177806
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_COMBO_SMSDRV,combobox,1344340227
Control8=IDC_STATIC_PORT,static,1342308352
Control9=IDC_COMBO_PORT,combobox,1344340227
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342177296
Control12=IDC_STATIC,static,1342308352
Control13=IDC_STATIC,static,1342177296

[DLG:IDD_SUMMARY]
Type=1
Class=CSummaryPage
ControlCount=3
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC,static,1342308352
Control3=IDC_EDIT_SUMMARY,edit,1352665284

[DLG:IDD_PROCESSING]
Type=1
Class=CProcessingPage
ControlCount=3
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC_STATUS,static,1342308352
Control3=IDC_LIST_STATUS,SysListView32,1342291969

[DLG:IDD_SRV_DEPS]
Type=1
Class=CSrvDepsPage
ControlCount=7
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC,static,1342308352
Control3=IDC_LIST_SERVICES,SysListView32,1342259225
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352

[CLS:CSrvDepsPage]
Type=0
HeaderFile=SrvDepsPage.h
ImplementationFile=SrvDepsPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=CSrvDepsPage

[DLG:IDD_SRV_DEPS (English (U.S.))]
Type=1
Class=?
ControlCount=7
Control1=IDC_STATIC,static,1342177806
Control2=IDC_STATIC,static,1342308352
Control3=IDC_LIST_SERVICES,SysListView32,1342259225
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352

