#if !defined(AFX_LASTVALUESVIEW_H__EBFC6420_6331_461B_B1E3_6457183A4DBB__INCLUDED_)
#define AFX_LASTVALUESVIEW_H__EBFC6420_6331_461B_B1E3_6457183A4DBB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LastValuesView.h : header file
//

#include "DynamicView.h"


/////////////////////////////////////////////////////////////////////////////
// CLastValuesView window

class CLastValuesView : public CDynamicView
{
// Construction
public:
	CLastValuesView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLastValuesView)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual void InitView(void *pData);
	virtual QWORD GetFingerprint(void);
	virtual ~CLastValuesView();

	// Generated message map functions
protected:
	CListCtrl m_wndListCtrl;
	DWORD m_dwNodeId;
	//{{AFX_MSG(CLastValuesView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewRefresh();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LASTVALUESVIEW_H__EBFC6420_6331_461B_B1E3_6457183A4DBB__INCLUDED_)
