; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
ClassCount=5
ResourceCount=2
NewFileInclude1=#include "stdafx.h"
Class1=CLoginDialog
LastClass=CSimpleListCtrl
Resource1=IDD_LOGIN (English (U.S.))
Class2=CAlarmSoundDlg
LastTemplate=CListCtrl
Class3=CFlatButton
Class4=CScintillaCtrl
Class5=CSimpleListCtrl
Resource2=IDD_ALARM_SOUNDS (English (U.S.))

[DLG:IDD_LOGIN (English (U.S.))]
Type=1
Class=CLoginDialog
ControlCount=18
Control1=IDC_EDIT_SERVER,edit,1350631552
Control2=IDC_EDIT_LOGIN,edit,1350631552
Control3=IDC_EDIT_PASSWORD,edit,1350631584
Control4=IDC_CHECK_ENCRYPT,button,1342242819
Control5=IDC_CHECK_CACHE,button,1342242819
Control6=IDC_CHECK_NOCACHE,button,1342242819
Control7=IDC_CHECK_VERSION_MATCH,button,1342242819
Control8=IDOK,button,1342242817
Control9=IDCANCEL,button,1342242816
Control10=IDC_STATIC,static,1342179342
Control11=IDC_STATIC,static,1342308352
Control12=IDC_STATIC,static,1342177296
Control13=IDC_STATIC,static,1342308352
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,static,1342308352
Control16=IDC_STATIC_VERSION,static,1342308482
Control17=IDC_STATIC,static,1342177296
Control18=IDC_STATIC,static,1342308352

[CLS:CLoginDialog]
Type=0
HeaderFile=LoginDialog.h
ImplementationFile=LoginDialog.cpp
BaseClass=CDialog
Filter=D
LastObject=CLoginDialog
VirtualFilter=dWC

[DLG:IDD_ALARM_SOUNDS (English (U.S.))]
Type=1
Class=CAlarmSoundDlg
ControlCount=20
Control1=IDC_RADIO_NO_SOUND,button,1342373897
Control2=IDC_RADIO_SOUND,button,1342242825
Control3=IDC_COMBO_SOUND1,combobox,1344340226
Control4=IDC_BROWSE_1,button,1342242816
Control5=IDC_BUTTON_PLAY_1,button,1342242880
Control6=IDC_COMBO_SOUND2,combobox,1344340226
Control7=IDC_BROWSE_2,button,1342242816
Control8=IDC_BUTTON_PLAY_2,button,1342242880
Control9=IDC_RADIO_SPEECH,button,1342242825
Control10=IDC_CHECK_SEVERITY,button,1342242819
Control11=IDC_CHECK_SOURCE,button,1342242819
Control12=IDC_CHECK_MESSAGE,button,1342242819
Control13=IDC_CHECK_ACK,button,1342252035
Control14=IDOK,button,1342242817
Control15=IDCANCEL,button,1342242816
Control16=IDC_STATIC_NEW_ALARM,static,1342308352
Control17=IDC_STATIC_ALARM_ACK,static,1342308352
Control18=IDC_STATIC,static,1342177296
Control19=IDC_STATIC,static,1342177296
Control20=IDC_STATIC,static,1342177296

[CLS:CAlarmSoundDlg]
Type=0
HeaderFile=AlarmSoundDlg.h
ImplementationFile=AlarmSoundDlg.cpp
BaseClass=CDialog
Filter=D
LastObject=CAlarmSoundDlg
VirtualFilter=dWC

[CLS:CFlatButton]
Type=0
HeaderFile=FlatButton.h
ImplementationFile=FlatButton.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC
LastObject=CFlatButton

[CLS:CScintillaCtrl]
Type=0
HeaderFile=ScintillaCtrl.h
ImplementationFile=ScintillaCtrl.cpp
BaseClass=generic CWnd
Filter=W

[CLS:CSimpleListCtrl]
Type=0
HeaderFile=SimpleListCtrl.h
ImplementationFile=SimpleListCtrl.cpp
BaseClass=CListCtrl
Filter=W
VirtualFilter=FWC

