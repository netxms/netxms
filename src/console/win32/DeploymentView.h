#if !defined(AFX_DEPLOYMENTVIEW_H__AA3A80C6_AA65_4D60_A4CF_930B055CA6C8__INCLUDED_)
#define AFX_DEPLOYMENTVIEW_H__AA3A80C6_AA65_4D60_A4CF_930B055CA6C8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DeploymentView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDeploymentView frame

class CDeploymentView : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CDeploymentView)
protected:
	CDeploymentView();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDeploymentView)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CListCtrl m_wndListCtrl;
	virtual ~CDeploymentView();

	// Generated message map functions
	//{{AFX_MSG(CDeploymentView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
   afx_msg void OnStartDeployment(WPARAM wParam, DEPLOYMENT_JOB *pJob);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DEPLOYMENTVIEW_H__AA3A80C6_AA65_4D60_A4CF_930B055CA6C8__INCLUDED_)
