#if !defined(AFX_CONTROLPANEL_H__86BA708D_C88D_40EE_A6E3_EA8A8B7D464E__INCLUDED_)
#define AFX_CONTROLPANEL_H__86BA708D_C88D_40EE_A6E3_EA8A8B7D464E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ControlPanel.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CControlPanel frame

class CControlPanel : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CControlPanel)
protected:
	CControlPanel();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CControlPanel)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	CImageList *m_pImageList;
	CListCtrl m_wndListCtrl;
	virtual ~CControlPanel();

	// Generated message map functions
	//{{AFX_MSG(CControlPanel)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONTROLPANEL_H__86BA708D_C88D_40EE_A6E3_EA8A8B7D464E__INCLUDED_)
