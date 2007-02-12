#if !defined(AFX_DEFINEGRAPHDLG_H__0C8AA24D_C922_4275_A904_DC81CCFA4B25__INCLUDED_)
#define AFX_DEFINEGRAPHDLG_H__0C8AA24D_C922_4275_A904_DC81CCFA4B25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DefineGraphDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDefineGraphDlg dialog

class CDefineGraphDlg : public CDialog
{
// Construction
public:
	NXC_GRAPH_ACL_ENTRY * m_pACL;
	DWORD m_dwACLSize;
	CDefineGraphDlg(CWnd* pParent = NULL);   // standard constructor
	~CDefineGraphDlg();

// Dialog Data
	//{{AFX_DATA(CDefineGraphDlg)
	enum { IDD = IDD_DEFINE_GRAPH };
	CListCtrl	m_wndListCtrl;
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDefineGraphDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	DWORD m_dwCurrAclEntry;
	void AddListEntry(NXC_GRAPH_ACL_ENTRY *pEntry);
	CImageList m_imageList;

	// Generated message map functions
	//{{AFX_MSG(CDefineGraphDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	afx_msg void OnItemchangedListUsers(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCheckRead();
	afx_msg void OnCheckModify();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEFINEGRAPHDLG_H__0C8AA24D_C922_4275_A904_DC81CCFA4B25__INCLUDED_)
