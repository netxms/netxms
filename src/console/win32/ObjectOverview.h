#if !defined(AFX_OBJECTOVERVIEW_H__267B4997_AB19_47FA_AAA6_6198E917C854__INCLUDED_)
#define AFX_OBJECTOVERVIEW_H__267B4997_AB19_47FA_AAA6_6198E917C854__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectOverview.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectOverview window

class CObjectOverview : public CWnd
{
// Construction
public:
	CObjectOverview();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectOverview)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectOverview();

	// Generated message map functions
protected:
	CFont m_fontBold;
	CFont m_fontNormal;
	NXC_OBJECT * m_pObject;
	//{{AFX_MSG(CObjectOverview)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG
   afx_msg void OnSetObject(WPARAM wParam, NXC_OBJECT *pObject);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTOVERVIEW_H__267B4997_AB19_47FA_AAA6_6198E917C854__INCLUDED_)
