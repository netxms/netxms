#if !defined(AFX_TRAPPARAMDLG_H__476CD6B2_A577_4CA5_9190_157E15425BAE__INCLUDED_)
#define AFX_TRAPPARAMDLG_H__476CD6B2_A577_4CA5_9190_157E15425BAE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrapParamDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrapParamDlg dialog

class CTrapParamDlg : public CDialog
{
// Construction
public:
	CTrapParamDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTrapParamDlg)
	enum { IDD = IDD_EDIT_TRAP_ARG };
	CEdit	m_wndEditOID;
	CString	m_strDescription;
	CString	m_strOID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrapParamDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTrapParamDlg)
	afx_msg void OnSelectOid();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAPPARAMDLG_H__476CD6B2_A577_4CA5_9190_157E15425BAE__INCLUDED_)
