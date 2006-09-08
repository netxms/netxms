#if !defined(AFX_CREATENODEDLG_H__5D2D2AE9_BE69_4D01_A87A_4E35CBA3059D__INCLUDED_)
#define AFX_CREATENODEDLG_H__5D2D2AE9_BE69_4D01_A87A_4E35CBA3059D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateNodeDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateNodeDlg dialog

class CCreateNodeDlg : public CCreateObjectDlg
{
// Construction
public:
	DWORD m_dwProxyNode;
	DWORD m_dwIpAddr;
	CCreateNodeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateNodeDlg)
	enum { IDD = IDD_CREATE_NODE };
	CEdit	m_wndObjectName;
	CIPAddressCtrl	m_wndIPAddr;
	BOOL	m_bDisableAgent;
	BOOL	m_bDisableICMP;
	BOOL	m_bDisableSNMP;
	BOOL	m_bCreateUnmanaged;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateNodeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateNodeDlg)
	virtual void OnOK();
	afx_msg void OnButtonResolve();
	afx_msg void OnSelectProxy();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATENODEDLG_H__5D2D2AE9_BE69_4D01_A87A_4E35CBA3059D__INCLUDED_)
