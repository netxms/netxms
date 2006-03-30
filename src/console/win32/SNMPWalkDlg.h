#if !defined(AFX_SNMPWALKDLG_H__443C16A5_E7D7_4853_A447_AAEE76A561CE__INCLUDED_)
#define AFX_SNMPWALKDLG_H__443C16A5_E7D7_4853_A447_AAEE76A561CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SNMPWalkDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSNMPWalkDlg dialog

class CSNMPWalkDlg : public CDialog
{
// Construction
public:
	DWORD m_dwObjectId;
	CString m_strRootOID;
	CString m_strNode;
	CSNMPWalkDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSNMPWalkDlg)
	enum { IDD = IDD_SNMP_WALK };
	CListCtrl	m_wndListCtrl;
	CStatic	m_wndStatus;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSNMPWalkDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSNMPWalkDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
   afx_msg void OnRequestCompleted(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SNMPWALKDLG_H__443C16A5_E7D7_4853_A447_AAEE76A561CE__INCLUDED_)
