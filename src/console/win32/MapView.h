#if !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
#define AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapView window

class CMapView : public CListCtrl
{
// Construction
public:
	CMapView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapView)
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMapView();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMapView)
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
