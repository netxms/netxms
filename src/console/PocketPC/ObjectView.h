#if !defined(AFX_OBJECTVIEW_H__9122B452_70C9_48BA_802A_34201206A20E__INCLUDED_)
#define AFX_OBJECTVIEW_H__9122B452_70C9_48BA_802A_34201206A20E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectView window

class CObjectView : public CWnd
{
// Construction
public:
	CObjectView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectView)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectView();

	// Generated message map functions
protected:
	CTreeCtrl m_wndTreeCtrl;
	//{{AFX_MSG(CObjectView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTVIEW_H__9122B452_70C9_48BA_802A_34201206A20E__INCLUDED_)
