#if !defined(AFX_MIBBROWSERDLG_H__A78EC728_4A40_4747_B6A6_9568A2194C66__INCLUDED_)
#define AFX_MIBBROWSERDLG_H__A78EC728_4A40_4747_B6A6_9568A2194C66__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MIBBrowserDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMIBBrowserDlg dialog

class CMIBBrowserDlg : public CDialog
{
// Construction
public:
	CMIBBrowserDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMIBBrowserDlg)
	enum { IDD = IDD_MIB_BROWSER };
	CEdit	m_wndEditType;
	CEdit	m_wndEditStatus;
	CEdit	m_wndEditOIDText;
	CEdit	m_wndEditAccess;
	CButton	m_wndButtonDetails;
	CEdit	m_wndEditDescription;
	CTreeCtrl	m_wndTreeCtrl;
	CEdit	m_wndEditOID;
	CString	m_strOID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMIBBrowserDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMIBBrowserDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangedTreeMib(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonDetails();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void ShowExtControls(BOOL bShow);
	CSize m_sizeCollapsed;
	CSize m_sizeExpanded;
	void Collapse(void);
	void Expand(void);
	BOOL m_bIsExpanded;
	struct tree m_mibTreeRoot;
	void AddTreeNode(HTREEITEM hRoot, struct tree *pCurrNode);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MIBBROWSERDLG_H__A78EC728_4A40_4747_B6A6_9568A2194C66__INCLUDED_)
