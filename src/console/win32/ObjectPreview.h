#if !defined(AFX_OBJECTPREVIEW_H__CEB3DD79_8E97_45C7_A986_7C29D0B2F506__INCLUDED_)
#define AFX_OBJECTPREVIEW_H__CEB3DD79_8E97_45C7_A986_7C29D0B2F506__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectPreview.h : header file
//

#include "ToolBox.h"

/////////////////////////////////////////////////////////////////////////////
// CObjectPreview window

class CObjectPreview : public CWnd
{
// Construction
public:
	CObjectPreview();

// Attributes
public:

protected:
   CToolBox m_wndObjectPreview;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectPreview)
	public:
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CObjectPreview();

   virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);

	// Generated message map functions
protected:
	//{{AFX_MSG(CObjectPreview)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTPREVIEW_H__CEB3DD79_8E97_45C7_A986_7C29D0B2F506__INCLUDED_)
