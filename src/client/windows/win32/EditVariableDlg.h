#if !defined(AFX_EDITVARIABLEDLG_H__714E7EC9_57FC_4C61_B396_EB65FD3FB954__INCLUDED_)
#define AFX_EDITVARIABLEDLG_H__714E7EC9_57FC_4C61_B396_EB65FD3FB954__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EditVariableDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CEditVariableDlg dialog

class CEditVariableDlg : public CDialog
{
// Construction
public:
	const TCHAR * m_pszTitle;
	BOOL m_bNewVariable;
	CEditVariableDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEditVariableDlg)
	enum { IDD = IDD_EDIT_VARIABLE };
	CString	m_strName;
	CString	m_strValue;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEditVariableDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEditVariableDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EDITVARIABLEDLG_H__714E7EC9_57FC_4C61_B396_EB65FD3FB954__INCLUDED_)
