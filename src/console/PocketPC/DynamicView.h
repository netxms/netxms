#if !defined(AFX_DYNAMICVIEW_H__7A1CC6E5_2F7A_49E1_9FA9_D7C9396791B6__INCLUDED_)
#define AFX_DYNAMICVIEW_H__7A1CC6E5_2F7A_49E1_9FA9_D7C9396791B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DynamicView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDynamicView window

class CDynamicView : public CWnd
{
// Construction
public:
	CDynamicView();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDynamicView)
	protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual void InitView(void *pData);
	virtual QWORD GetFingerprint(void);
	virtual ~CDynamicView();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDynamicView)
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DYNAMICVIEW_H__7A1CC6E5_2F7A_49E1_9FA9_D7C9396791B6__INCLUDED_)
