#if !defined(AFX_NETSUMMARYFRAME_H__EBCECD30_3D1F_447D_83EF_AA0634294F43__INCLUDED_)
#define AFX_NETSUMMARYFRAME_H__EBCECD30_3D1F_447D_83EF_AA0634294F43__INCLUDED_

#include "NodeSummary.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NetSummaryFrame.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNetSummaryFrame frame

class CNetSummaryFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CNetSummaryFrame)
protected:
	CNetSummaryFrame();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetSummaryFrame)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CNodeSummary m_wndNodeSummary;
	virtual ~CNetSummaryFrame();

	// Generated message map functions
	//{{AFX_MSG(CNetSummaryFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnViewRefresh();
	afx_msg void OnClose();
	//}}AFX_MSG
   afx_msg void OnObjectChange(DWORD dwObjectId, NXC_OBJECT *pObject);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NETSUMMARYFRAME_H__EBCECD30_3D1F_447D_83EF_AA0634294F43__INCLUDED_)
