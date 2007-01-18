#if !defined(AFX_CLUSTERVIEW_H__0BBE0D41_D477_41C3_8DBB_81DF54FAD41E__INCLUDED_)
#define AFX_CLUSTERVIEW_H__0BBE0D41_D477_41C3_8DBB_81DF54FAD41E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ClusterView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CClusterView window

class CClusterView : public CWnd
{
// Construction
public:
	CClusterView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClusterView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CClusterView();

	// Generated message map functions
protected:
	NXC_OBJECT * m_pObject;
	//{{AFX_MSG(CClusterView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG
   afx_msg void OnSetObject(WPARAM wParam, NXC_OBJECT *pObject);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLUSTERVIEW_H__0BBE0D41_D477_41C3_8DBB_81DF54FAD41E__INCLUDED_)
