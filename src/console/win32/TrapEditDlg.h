#if !defined(AFX_TRAPEDITDLG_H__BD37EEA6_B0D1_40D1_A12F_36F46CD76DC6__INCLUDED_)
#define AFX_TRAPEDITDLG_H__BD37EEA6_B0D1_40D1_A12F_36F46CD76DC6__INCLUDED_

#include "..\..\..\INCLUDE\nxclapi.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrapEditDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrapEditDlg dialog

class CTrapEditDlg : public CDialog
{
// Construction
public:
	NXC_TRAP_CFG_ENTRY m_trap;
	CTrapEditDlg(CWnd* pParent = NULL);   // standard constructor
   ~CTrapEditDlg();

// Dialog Data
	//{{AFX_DATA(CTrapEditDlg)
	enum { IDD = IDD_EDIT_TRAP };
	CEdit	m_wndEditDescr;
	CEdit	m_wndEditOID;
	CEdit	m_wndEditEvent;
	CEdit	m_wndEditMsg;
	CListCtrl	m_wndArgList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrapEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTrapEditDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAPEDITDLG_H__BD37EEA6_B0D1_40D1_A12F_36F46CD76DC6__INCLUDED_)
