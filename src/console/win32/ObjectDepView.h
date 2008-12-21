#if !defined(AFX_OBJECTDEPVIEW_H__C35C789A_82CA_465E_95D9_8DC327F5851E__INCLUDED_)
#define AFX_OBJECTDEPVIEW_H__C35C789A_82CA_465E_95D9_8DC327F5851E__INCLUDED_

#include "MapView.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectDepView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectDepView window

class CObjectDepView : public CWnd
{
// Construction
public:
	CObjectDepView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectDepView)
	//}}AFX_VIRTUAL

// Implementation
public:
	void Refresh(void);
	virtual ~CObjectDepView();

	// Generated message map functions
protected:
	void AddParentsToMap(NXC_OBJECT *pChild, nxmap_ObjList &list);
	NXC_OBJECT * m_pObject;
	CMapView m_wndMap;
	//{{AFX_MSG(CObjectDepView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
   afx_msg void OnSetObject(WPARAM wParam, NXC_OBJECT *pObject);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTDEPVIEW_H__C35C789A_82CA_465E_95D9_8DC327F5851E__INCLUDED_)
