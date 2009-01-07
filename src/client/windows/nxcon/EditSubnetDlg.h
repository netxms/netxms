#if !defined(AFX_EDITSUBNETDLG_H__DCEA12C2_646B_495E_AC2C_7EFF3D61C069__INCLUDED_)
#define AFX_EDITSUBNETDLG_H__DCEA12C2_646B_495E_AC2C_7EFF3D61C069__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditSubnetDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditSubnetDlg dialog

class CEditSubnetDlg : public CDialog
{
// Construction
public:
	IP_NETWORK m_subnet;
	CEditSubnetDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditSubnetDlg)
	enum { IDD = IDD_EDIT_IP_SUBNET };
	CIPAddressCtrl	m_wndIPNetMask;
	CIPAddressCtrl	m_wndIPAddr;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditSubnetDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditSubnetDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITSUBNETDLG_H__DCEA12C2_646B_495E_AC2C_7EFF3D61C069__INCLUDED_)
