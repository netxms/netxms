#if !defined(AFX_MAPTOOLBOX_H__0F1C1940_84D6_4E4F_AA55_A5CBFA61474F__INCLUDED_)
#define AFX_MAPTOOLBOX_H__0F1C1940_84D6_4E4F_AA55_A5CBFA61474F__INCLUDED_

#include "MapControlBox.h"	// Added by ClassView


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapToolbox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapToolbox window

class CMapToolbox : public CWnd
{
// Construction
public:
	CMapToolbox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapToolbox)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMapToolbox();

	// Generated message map functions
protected:
	CMapControlBox m_wndControlBox;
	//{{AFX_MSG(CMapToolbox)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPTOOLBOX_H__0F1C1940_84D6_4E4F_AA55_A5CBFA61474F__INCLUDED_)
