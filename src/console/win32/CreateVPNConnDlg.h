#if !defined(AFX_CREATEVPNCONNDLG_H__E7F99B5F_5ACF_40E8_BE6B_7DAEE155EDF3__INCLUDED_)
#define AFX_CREATEVPNCONNDLG_H__E7F99B5F_5ACF_40E8_BE6B_7DAEE155EDF3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateVPNConnDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateVPNConnDlg dialog

class CCreateVPNConnDlg : public CCreateObjectDlg
{
// Construction
public:
	CCreateVPNConnDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateVPNConnDlg)
	enum { IDD = IDD_CREATE_VPNC };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateVPNConnDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateVPNConnDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATEVPNCONNDLG_H__E7F99B5F_5ACF_40E8_BE6B_7DAEE155EDF3__INCLUDED_)
