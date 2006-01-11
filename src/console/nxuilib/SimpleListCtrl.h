#if !defined(AFX_SIMPLELISTCTRL_H__34F6CC19_35DF_41B6_A8B5_9809789B1FFE__INCLUDED_)
#define AFX_SIMPLELISTCTRL_H__34F6CC19_35DF_41B6_A8B5_9809789B1FFE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SimpleListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSimpleListCtrl window

class NXUILIB_EXPORTABLE CSimpleListCtrl : public CListCtrl
{
// Construction
public:
	CSimpleListCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimpleListCtrl)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSimpleListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSimpleListCtrl)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIMPLELISTCTRL_H__34F6CC19_35DF_41B6_A8B5_9809789B1FFE__INCLUDED_)
