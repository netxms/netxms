#if !defined(AFX_INTERNALITEMSELDLG_H__7FB54353_A470_4928_B35C_9532A93FDAC9__INCLUDED_)
#define AFX_INTERNALITEMSELDLG_H__7FB54353_A470_4928_B35C_9532A93FDAC9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// InternalItemSelDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInternalItemSelDlg dialog

class CInternalItemSelDlg : public CDialog
{
// Construction
public:
	int m_iDataType;
	TCHAR m_szItemDescription[MAX_DB_STRING];
	TCHAR m_szItemName[MAX_DB_STRING];
	NXC_OBJECT *m_pNode;
	CInternalItemSelDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CInternalItemSelDlg)
	enum { IDD = IDD_SELECT_INTERNAL_ITEM };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CInternalItemSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CInternalItemSelDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnButtonGet();
	afx_msg void OnDblclkListParameters(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INTERNALITEMSELDLG_H__7FB54353_A470_4928_B35C_9532A93FDAC9__INCLUDED_)
