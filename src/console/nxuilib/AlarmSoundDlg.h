#if !defined(AFX_ALARMSOUNDDLG_H__9EEE8F77_A307_48F1_8E49_152F5A9D01DE__INCLUDED_)
#define AFX_ALARMSOUNDDLG_H__9EEE8F77_A307_48F1_8E49_152F5A9D01DE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AlarmSoundDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAlarmSoundDlg dialog

class CAlarmSoundDlg : public CDialog
{
// Construction
public:
	CAlarmSoundDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAlarmSoundDlg)
	enum { IDD = IDD_ALARM_SOUNDS };
	CButton	m_wndButtonPlay2;
	CButton	m_wndButtonPlay1;
	CComboBox	m_wndCombo2;
	CComboBox	m_wndCombo1;
	int		m_iSoundType;
	CString	m_strSound1;
	CString	m_strSound2;
	BOOL	m_bIncludeSource;
	BOOL	m_bIncludeSeverity;
	BOOL	m_bIncludeMessage;
	BOOL	m_bNotifyOnAck;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAlarmSoundDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void PlayDemoSound(TCHAR *pszSound);
	void EnableItems(int nSoundType);
	void SelectFile(int nCtrl);

	// Generated message map functions
	//{{AFX_MSG(CAlarmSoundDlg)
	afx_msg void OnBrowse1();
	afx_msg void OnBrowse2();
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioNoSound();
	afx_msg void OnRadioSound();
	afx_msg void OnRadioSpeech();
	afx_msg void OnButtonPlay1();
	afx_msg void OnButtonPlay2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMSOUNDDLG_H__9EEE8F77_A307_48F1_8E49_152F5A9D01DE__INCLUDED_)
