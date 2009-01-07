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
	DWORD * m_pdwMembers;
	DWORD m_dwNumMembers;
	NXC_USER * m_pGroup;
	CGroupPropDlg(CWnd* pParent = NULL);   // standard constructor
   virtual ~CGroupPropDlg();

// Dialog Data
	//{{AFX_DATA(CGroupPropDlg)
	enum { IDD = IDD_GROUP_PROPERTIES };
	CListCtrl	m_wndListCtrl;
	BOOL	m_bDisabled;
	BOOL	m_bDropConn;
	BOOL	m_bEditEventDB;
	BOOL	m_bManageUsers;
	BOOL	m_bViewEventDB;
	CString	m_strDescription;
	CString	m_strName;
	BOOL	m_bManageActions;
	BOOL	m_bManageEPP;
	BOOL	m_bManageConfig;
	BOOL	m_bConfigureTraps;
	BOOL	m_bManagePkg;
	BOOL	m_bDeleteAlarms;
	BOOL	m_bManageAgentCfg;
	BOOL	m_bManageScripts;
	BOOL	m_bManageTools;
	BOOL	m_bViewTrapLog;
	BOOL	m_bAccessFiles;
	BOOL	m_bRegisterAgents;
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
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GROUPPROPDLG_H__BA37A828_FFAF_4479_9F52_CE34C0244F93__INCLUDED_)
