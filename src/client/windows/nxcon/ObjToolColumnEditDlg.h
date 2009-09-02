#if !defined(AFX_OBJTOOLCOLUMNEDITDLG_H__1893360A_4B91_418A_9152_2209B8A11A9A__INCLUDED_)
#define AFX_OBJTOOLCOLUMNEDITDLG_H__1893360A_4B91_418A_9152_2209B8A11A9A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjToolColumnEditDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjToolColumnEditDlg dialog

class CObjToolColumnEditDlg : public CDialog
{
// Construction
public:
	BOOL m_isAgentTable;
	BOOL m_isCreate;
	CObjToolColumnEditDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CObjToolColumnEditDlg)
	enum { IDD = IDD_EDIT_COLUMN };
	CComboBox	m_wndComboType;
	int		m_type;
	CString	m_name;
	CString	m_oid;
	CString	m_oidLabel;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjToolColumnEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjToolColumnEditDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJTOOLCOLUMNEDITDLG_H__1893360A_4B91_418A_9152_2209B8A11A9A__INCLUDED_)
