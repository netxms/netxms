#if !defined(AFX_CONDDCIPROPDLG_H__0919D775_92D6_4086_B954_5C7359DDD993__INCLUDED_)
#define AFX_CONDDCIPROPDLG_H__0919D775_92D6_4086_B954_5C7359DDD993__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CondDCIPropDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCondDCIPropDlg dialog

class CCondDCIPropDlg : public CDialog
{
// Construction
public:
	int m_nFunction;
	CCondDCIPropDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCondDCIPropDlg)
	enum { IDD = IDD_COND_DCI_PROP };
	CComboBox	m_wndComboFunc;
	int		m_nPolls;
	CString	m_strItem;
	CString	m_strNode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCondDCIPropDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCondDCIPropDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeComboFunction();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONDDCIPROPDLG_H__0919D775_92D6_4086_B954_5C7359DDD993__INCLUDED_)
