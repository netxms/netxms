#if !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
#define AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MapView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMapView window

class CMapView : public CWnd
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
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMapView();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMapView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPVIEW_H__9082BB7F_2C80_4E6E_9FDB_A4D4039C2CB1__INCLUDED_)
