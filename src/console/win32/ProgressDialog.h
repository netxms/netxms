#if !defined(AFX_PROGRESSDIALOG_H__E38295A1_D9DF_4FEE_9E53_4585E17DBD5B__INCLUDED_)
#define AFX_PROGRESSDIALOG_H__E38295A1_D9DF_4FEE_9E53_4585E17DBD5B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProgressDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProgressDialog dialog

class CProgressDialog : public CDialog
{
// Construction
public:
	CProgressDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CProgressDialog)
	enum { IDD = IDD_PROGRESS };
	CString	m_szStatusText;
	//}}AFX_DATA

   void Terminate(int iCode) { PostMessage(WM_CLOSE_STATUS_DLG, 0, iCode); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CProgressDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	afx_msg LRESULT OnCloseStatusDlg(WPARAM wParam, LPARAM lParam);

	// Generated message map functions
	//{{AFX_MSG(CProgressDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PROGRESSDIALOG_H__E38295A1_D9DF_4FEE_9E53_4585E17DBD5B__INCLUDED_)
