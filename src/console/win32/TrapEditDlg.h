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
	CStatic	m_wndEventIcon;
	CEdit	m_wndEditOID;
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
	void UpdateParameterEntry(DWORD dwIndex);
	void AddParameterEntry(DWORD dwIndex);

	// Generated message map functions
	//{{AFX_MSG(CTrapEditDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelectTrap();
	virtual void OnOK();
	afx_msg void OnSelectEvent();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	afx_msg void OnButtonEdit();
	afx_msg void OnButtonUp();
	afx_msg void OnButtonDown();
	afx_msg void OnItemchangedListArgs(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkListArgs(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void UpdateEventInfo(void);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAPEDITDLG_H__BD37EEA6_B0D1_40D1_A12F_36F46CD76DC6__INCLUDED_)
