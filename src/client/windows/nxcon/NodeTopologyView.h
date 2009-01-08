#if !defined(AFX_NODETOPOLOGYVIEW_H__DD1A60B3_4A79_4626_8D2A_D79E16839976__INCLUDED_)
#define AFX_NODETOPOLOGYVIEW_H__DD1A60B3_4A79_4626_8D2A_D79E16839976__INCLUDED_

#include "MapView.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NodeTopologyView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNodeTopologyView window

class CNodeTopologyView : public CWnd
{
// Construction
public:
	CNodeTopologyView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodeTopologyView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNodeTopologyView();

	// Generated message map functions
protected:
	int m_nState;
	NXC_OBJECT * m_pObject;
	CMapView m_wndMap;
	//{{AFX_MSG(CNodeTopologyView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnPaint();
	//}}AFX_MSG
	afx_msg LRESULT OnSetObject(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRequestCompleted(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

public:
	DWORD GetObjectId(void) { return m_pObject != NULL ? m_pObject->dwId : 0; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NODETOPOLOGYVIEW_H__DD1A60B3_4A79_4626_8D2A_D79E16839976__INCLUDED_)
