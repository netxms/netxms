#if !defined(AFX_CREATECONTAINERDLG_H__85C493EB_B006_43C7_9938_30299BBACEFE__INCLUDED_)
#define AFX_CREATECONTAINERDLG_H__85C493EB_B006_43C7_9938_30299BBACEFE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateContainerDlg.h : header file
//

#include "CreateObjectDlg.h"


/////////////////////////////////////////////////////////////////////////////
// CCreateContainerDlg dialog

class CCreateContainerDlg : public CCreateObjectDlg
{
// Construction
public:
	CCreateContainerDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateContainerDlg)
	enum { IDD = IDD_CREATE_CONTAINER };
	CComboBox	m_wndCategoryList;
	CString	m_strDescription;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateContainerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateContainerDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATECONTAINERDLG_H__85C493EB_B006_43C7_9938_30299BBACEFE__INCLUDED_)
