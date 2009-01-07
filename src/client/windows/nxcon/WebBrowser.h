#if !defined(AFX_WEBBROWSER_H__31FE3E37_120D_4728_B038_96566B59A644__INCLUDED_)
#define AFX_WEBBROWSER_H__31FE3E37_120D_4728_B038_96566B59A644__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WebBrowser.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWebBrowser frame

class CWebBrowser : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CWebBrowser)
protected:
	CWebBrowser();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
   CWebBrowser(TCHAR *pszStartURL);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWebBrowser)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CHtmlView *m_pwndBrowser;
	TCHAR *m_pszStartURL;
	virtual ~CWebBrowser();

	// Generated message map functions
	//{{AFX_MSG(CWebBrowser)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WEBBROWSER_H__31FE3E37_120D_4728_B038_96566B59A644__INCLUDED_)
