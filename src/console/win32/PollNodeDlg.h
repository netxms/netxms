#if !defined(AFX_POLLNODEDLG_H__7C404C49_8BF3_492B_8E8C_C53EAE704C76__INCLUDED_)
#define AFX_POLLNODEDLG_H__7C404C49_8BF3_492B_8E8C_C53EAE704C76__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PollNodeDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPollNodeDlg dialog

class CPollNodeDlg : public CDialog
{
// Construction
public:
	HWND *m_phWnd;
	HANDLE m_hThread;
	CPollNodeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPollNodeDlg)
	enum { IDD = IDD_POLL_NODE };
	CButton	m_wndCloseButton;
	CEdit	m_wndMsgBox;
	CProgressCtrl	m_wndProgressBar;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPollNodeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPollNodeDlg)
	virtual BOOL OnInitDialog();
	virtual void OnCancel();
	//}}AFX_MSG
   afx_msg void OnRequestCompleted(WPARAM wParam, LPARAM lParam);
   afx_msg void OnPollerMessage(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
private:
	CFont m_font;
	DWORD m_dwResult;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POLLNODEDLG_H__7C404C49_8BF3_492B_8E8C_C53EAE704C76__INCLUDED_)
