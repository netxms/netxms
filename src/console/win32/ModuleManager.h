#if !defined(AFX_MODULEMANAGER_H__B88D3066_4A71_422B_9552_5B936FA8A5A9__INCLUDED_)
#define AFX_MODULEMANAGER_H__B88D3066_4A71_422B_9552_5B936FA8A5A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ModuleManager.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CModuleManager frame

class CModuleManager : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CModuleManager)
protected:
	CModuleManager();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModuleManager)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CListCtrl m_wndListCtrl;
	virtual ~CModuleManager();

	// Generated message map functions
	//{{AFX_MSG(CModuleManager)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODULEMANAGER_H__B88D3066_4A71_422B_9552_5B936FA8A5A9__INCLUDED_)
