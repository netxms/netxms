#if !defined(AFX_RULESITUATIONDLG_H__EB569571_64C4_4E25_AD9E_F7CA2F850E96__INCLUDED_)
#define AFX_RULESITUATIONDLG_H__EB569571_64C4_4E25_AD9E_F7CA2F850E96__INCLUDED_

#include "..\..\..\INCLUDE\nms_util.h"	// Added by ClassView
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RuleSituationDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRuleSituationDlg dialog

class CRuleSituationDlg : public CDialog
{
// Construction
public:
	StringMap m_attrList;
	CString m_strInstance;
	DWORD m_dwSituation;
	CRuleSituationDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRuleSituationDlg)
	enum { IDD = IDD_EDIT_RULE_SITUATION };
	CListCtrl	m_wndListCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRuleSituationDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void EnableControls(BOOL enable);

	// Generated message map functions
	//{{AFX_MSG(CRuleSituationDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnCheckEnable();
	afx_msg void OnButtonSelect();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonDelete();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RULESITUATIONDLG_H__EB569571_64C4_4E25_AD9E_F7CA2F850E96__INCLUDED_)
