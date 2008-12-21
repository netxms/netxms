#if !defined(AFX_REQUESTPROCESSINGDLG_H__58B4CC16_1239_4835_91DA_791FAAAC9C1D__INCLUDED_)
#define AFX_REQUESTPROCESSINGDLG_H__58B4CC16_1239_4835_91DA_791FAAAC9C1D__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RequestProcessingDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRequestProcessingDlg dialog

class CRequestProcessingDlg : public CDialog
{
// Construction
public:
	HWND *m_phWnd;
	HANDLE m_hThread;
	CRequestProcessingDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRequestProcessingDlg)
	enum { IDD = IDD_REQUEST_PROCESSING };
	CStatic	m_wndInfoText;
	CString	m_strInfoText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRequestProcessingDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRequestProcessingDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
   afx_msg void OnRequestCompleted(WPARAM wParam, LPARAM lParam);
   afx_msg void OnSetInfoText(WPARAM wParam, LPARAM lParam);
   afx_msg LRESULT OnChangePassword(WPARAM wParam, TCHAR *pszBuffer);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REQUESTPROCESSINGDLG_H__58B4CC16_1239_4835_91DA_791FAAAC9C1D__INCLUDED_)
