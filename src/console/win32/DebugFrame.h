#if !defined(AFX_DEBUGFRAME_H__D462B85B_69F4_4EAF_81E0_1E1FBCF952DE__INCLUDED_)
#define AFX_DEBUGFRAME_H__D462B85B_69F4_4EAF_81E0_1E1FBCF952DE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DebugFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDebugFrame frame

class CDebugFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CDebugFrame)
protected:
	CDebugFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:
	void AddMessage(char *pszMsg);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDebugFrame)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CListCtrl m_wndListCtrl;
	virtual ~CDebugFrame();

	// Generated message map functions
	//{{AFX_MSG(CDebugFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEBUGFRAME_H__D462B85B_69F4_4EAF_81E0_1E1FBCF952DE__INCLUDED_)
