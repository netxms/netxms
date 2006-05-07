#if !defined(AFX_PASSWORDCHANGEDLG_H__B5257977_8E64_48F6_8E44_7260D1A5F42D__INCLUDED_)
#define AFX_PASSWORDCHANGEDLG_H__B5257977_8E64_48F6_8E44_7260D1A5F42D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PasswordChangeDlg.h : header file
//

#define MAX_PASSWORD_LENGTH      64

/////////////////////////////////////////////////////////////////////////////
// CPasswordChangeDlg dialog

class CPasswordChangeDlg : public CDialog
{
// Construction
public:
	char m_szPassword[MAX_PASSWORD_LENGTH];
	CPasswordChangeDlg(int nTemplate, CWnd* pParent = NULL);

// Dialog Data
	//{{AFX_DATA(CPasswordChangeDlg)
	enum { IDD = IDD_SET_PASSWORD };
	CEdit	m_wndEditBox2;
	CEdit	m_wndEditBox1;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPasswordChangeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPasswordChangeDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PASSWORDCHANGEDLG_H__B5257977_8E64_48F6_8E44_7260D1A5F42D__INCLUDED_)
