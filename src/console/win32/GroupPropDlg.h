#if !defined(AFX_GROUPPROPDLG_H__BA37A828_FFAF_4479_9F52_CE34C0244F93__INCLUDED_)
#define AFX_GROUPPROPDLG_H__BA37A828_FFAF_4479_9F52_CE34C0244F93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GroupPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGroupPropDlg dialog

class CGroupPropDlg : public CDialog
{
// Construction
public:
	CGroupPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGroupPropDlg)
	enum { IDD = IDD_GROUP_PROPERTIES };
	CListCtrl	m_wndListCtrl;
	BOOL	m_bDisabled;
	BOOL	m_bDropConn;
	BOOL	m_bEditConfig;
	BOOL	m_bEditEventDB;
	BOOL	m_bManageUsers;
	BOOL	m_bViewConfig;
	BOOL	m_bViewEventDB;
	CString	m_strDescription;
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupPropDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPPROPDLG_H__BA37A828_FFAF_4479_9F52_CE34C0244F93__INCLUDED_)
