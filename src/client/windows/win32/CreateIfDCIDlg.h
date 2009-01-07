#if !defined(AFX_CREATEIFDCIDLG_H__241BE627_0F55_413B_9AC7_2BEADDE0472B__INCLUDED_)
#define AFX_CREATEIFDCIDLG_H__241BE627_0F55_413B_9AC7_2BEADDE0472B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateIfDCIDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCreateIfDCIDlg dialog

class CCreateIfDCIDlg : public CDialog
{
// Construction
public:
	CCreateIfDCIDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateIfDCIDlg)
	enum { IDD = IDD_CREATE_IF_DCI };
	BOOL	m_bInErrors;
	BOOL	m_bOutErrors;
	BOOL	m_bInBytes;
	BOOL	m_bInPackets;
	BOOL	m_bOutBytes;
	BOOL	m_bOutPackets;
	CString	m_strInBytes;
	CString	m_strInErrors;
	CString	m_strInPackets;
	CString	m_strOutBytes;
	CString	m_strOutErrors;
	CString	m_strOutPackets;
	BOOL	m_bDeltaInBytes;
	BOOL	m_bDeltaInErrors;
	BOOL	m_bDeltaInPackets;
	BOOL	m_bDeltaOutBytes;
	BOOL	m_bDeltaOutErrors;
	BOOL	m_bDeltaOutPackets;
	DWORD	m_dwInterval;
	DWORD	m_dwRetention;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateIfDCIDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableControl(int nCtrlBase, int nCtrl1, int nCtrl2);
	void EnableControls(void);

	// Generated message map functions
	//{{AFX_MSG(CCreateIfDCIDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckOutp();
	afx_msg void OnCheckOut();
	afx_msg void OnCheckInp();
	afx_msg void OnCheckIn();
	afx_msg void OnCheckErrorsOut();
	afx_msg void OnCheckErrorsIn();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATEIFDCIDLG_H__241BE627_0F55_413B_9AC7_2BEADDE0472B__INCLUDED_)
