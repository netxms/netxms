; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
ClassCount=10
ResourceCount=6
NewFileInclude1=#include "stdafx.h"
Class1=CLoginDialog
LastClass=CLoginDialog
Resource1=IDD_LOGIN (English (U.S.))
Class2=CAlarmSoundDlg
LastTemplate=generic CWnd
Class3=CFlatButton
Class4=CScintillaCtrl
Class5=CSimpleListCtrl
Resource2=IDD_ALARM_SOUNDS (English (U.S.))
Resource3=IDD_NOTIFY (English (U.S.))
Resource4=IDD_LOGIN
Resource5=IDD_ALARM_SOUNDS
Class6=CTaskBarPopupWnd
Class7=CComboListCtrl
Class8=CInPlaceEdit
Class9=CInPlaceCombo
Class10=CColourPickerXP
Resource6=IDD_NOTIFY

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

[DLG:IDD_NOTIFY (English (U.S.))]
Type=1
Class=?
ControlCount=0

[DLG:IDD_LOGIN]
Type=1
Class=CLoginDialog
ControlCount=19
Control1=IDC_COMBO_SERVER,combobox,1344340226
Control2=IDC_EDIT_LOGIN,edit,1350631552
Control3=IDC_RADIO_PASSWORD,button,1342373897
Control4=IDC_RADIO_CERT,button,1342177289
Control5=IDC_EDIT_PASSWORD,edit,1350631584
Control6=IDC_COMBO_CERTS,combobox,1344339971
Control7=IDC_CHECK_ENCRYPT,button,1342242819
Control8=IDC_CHECK_CACHE,button,1342242819
Control9=IDC_CHECK_NOCACHE,button,1342242819
Control10=IDC_CHECK_VERSION_MATCH,button,1342242819
Control11=IDOK,button,1342242817
Control12=IDCANCEL,button,1342242816
Control13=IDC_STATIC,static,1342179854
Control14=IDC_STATIC,static,1342308352
Control15=IDC_STATIC,static,1342308352
Control16=IDC_STATIC_PASSWORD,static,1342308352
Control17=IDC_STATIC_VERSION,static,1342308482
Control18=IDC_STATIC,button,1342177287
Control19=IDC_STATIC,button,1342177287

[DLG:IDD_ALARM_SOUNDS]
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

[DLG:IDD_NOTIFY]
Type=1
Class=?
ControlCount=0

[CLS:CTaskBarPopupWnd]
Type=0
HeaderFile=TaskBarPopupWnd.h
ImplementationFile=TaskBarPopupWnd.cpp
BaseClass=CWnd
Filter=W
VirtualFilter=WC

[CLS:CComboListCtrl]
Type=0
HeaderFile=ComboListCtrl.h
ImplementationFile=ComboListCtrl.cpp
BaseClass=CListCtrl
Filter=W
VirtualFilter=FWC

[CLS:CInPlaceEdit]
Type=0
HeaderFile=InPlaceEdit.h
ImplementationFile=InPlaceEdit.cpp
BaseClass=CEdit
Filter=W
VirtualFilter=FWC

[CLS:CInPlaceCombo]
Type=0
HeaderFile=InPlaceCombo.h
ImplementationFile=InPlaceCombo.cpp
BaseClass=CComboBox
Filter=W
VirtualFilter=FWC

[CLS:CColourPickerXP]
Type=0
HeaderFile=ColourPickerXP.h
ImplementationFile=ColourPickerXP.cpp
BaseClass=CButton
Filter=W
VirtualFilter=FWC

