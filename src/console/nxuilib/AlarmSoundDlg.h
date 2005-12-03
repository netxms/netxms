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

	// Generated message map functions
	//{{AFX_MSG(CAlarmSoundDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ALARMSOUNDDLG_H__9EEE8F77_A307_48F1_8E49_152F5A9D01DE__INCLUDED_)
