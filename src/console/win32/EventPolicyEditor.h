#if !defined(AFX_EVENTPOLICYEDITOR_H__E3E2B03F_812E_4289_8595_F36FCD0072F8__INCLUDED_)
#define AFX_EVENTPOLICYEDITOR_H__E3E2B03F_812E_4289_8595_F36FCD0072F8__INCLUDED_

#include "RuleList.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EventPolicyEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventPolicyEditor frame

class CEventPolicyEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CEventPolicyEditor)
protected:
	CEventPolicyEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventPolicyEditor)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CRuleList m_wndRuleList;
	virtual ~CEventPolicyEditor();

	// Generated message map functions
	//{{AFX_MSG(CEventPolicyEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTPOLICYEDITOR_H__E3E2B03F_812E_4289_8595_F36FCD0072F8__INCLUDED_)
