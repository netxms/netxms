#if !defined(AFX_USEREDITOR_H__2D1C5AB7_A0ED_467D_A1F3_9ECACE44EE95__INCLUDED_)
#define AFX_USEREDITOR_H__2D1C5AB7_A0ED_467D_A1F3_9ECACE44EE95__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// UserEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUserEditor frame

class CUserEditor : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CUserEditor)
protected:
	CUserEditor();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserEditor)
	//}}AFX_VIRTUAL

// Implementation
protected:
	CListCtrl m_wndListCtrl;
	virtual ~CUserEditor();

	// Generated message map functions
	//{{AFX_MSG(CUserEditor)
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_USEREDITOR_H__2D1C5AB7_A0ED_467D_A1F3_9ECACE44EE95__INCLUDED_)
