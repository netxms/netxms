#if !defined(AFX_USERSELECTDLG_H__7DC29F9E_3B96_45B9_817D_1FE87E361848__INCLUDED_)
#define AFX_USERSELECTDLG_H__7DC29F9E_3B96_45B9_817D_1FE87E361848__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UserSelectDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUserSelectDlg dialog

class CUserSelectDlg : public CDialog
{
// Construction
public:
	DWORD m_dwUserId;
	CUserSelectDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CUserSelectDlg)
	enum { IDD = IDD_SELECT_USER };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserSelectDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CUserSelectDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblclkListUsers(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USERSELECTDLG_H__7DC29F9E_3B96_45B9_817D_1FE87E361848__INCLUDED_)
