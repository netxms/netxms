#if !defined(AFX_EVENTEDITOR_H__A45B4A38_99BC_4277_B600_742C39D58310__INCLUDED_)
#define AFX_EVENTEDITOR_H__A45B4A38_99BC_4277_B600_742C39D58310__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EventEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEventEditor frame

class CEventEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CEventEditor)
protected:
	CEventEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEventEditor)
	//}}AFX_VIRTUAL

// Implementation
protected:
	CListCtrl m_wndListCtrl;
	virtual ~CEventEditor();

	// Generated message map functions
	//{{AFX_MSG(CEventEditor)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTEDITOR_H__A45B4A38_99BC_4277_B600_742C39D58310__INCLUDED_)
