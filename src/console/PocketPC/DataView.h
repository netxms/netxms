#if !defined(AFX_DATAVIEW_H__AE957D6B_62A1_488F_8C5E_89DA9F6C5E74__INCLUDED_)
#define AFX_DATAVIEW_H__AE957D6B_62A1_488F_8C5E_89DA9F6C5E74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DataView.h : header file
//

#include "DynamicView.h"


//
// Init structure
//

struct DATA_VIEW_INIT
{
   DWORD dwNodeId;
   DWORD dwItemId;
};


/////////////////////////////////////////////////////////////////////////////
// CDataView window

class CDataView : public CDynamicView
{
// Construction
public:
	CDataView();

// Attributes
public:

// Operations
public:
	virtual void InitView(void *pData);
	virtual QWORD GetFingerprint(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDataView)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDataView();

	// Generated message map functions
protected:
	CListCtrl m_wndListCtrl;
	DWORD m_dwItemId;
	DWORD m_dwNodeId;
	//{{AFX_MSG(CDataView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnViewRefresh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DATAVIEW_H__AE957D6B_62A1_488F_8C5E_89DA9F6C5E74__INCLUDED_)
