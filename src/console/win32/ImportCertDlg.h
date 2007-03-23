#if !defined(AFX_IMPORTCERTDLG_H__AB4D238C_1028_4304_9043_09DB76692B74__INCLUDED_)
#define AFX_IMPORTCERTDLG_H__AB4D238C_1028_4304_9043_09DB76692B74__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ImportCertDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CImportCertDlg dialog

class CImportCertDlg : public CDialog
{
// Construction
public:
	CImportCertDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CImportCertDlg)
	enum { IDD = IDD_IMPORT_CA_CERT };
	CString	m_strComments;
	CString	m_strFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImportCertDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CImportCertDlg)
	afx_msg void OnButtonBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMPORTCERTDLG_H__AB4D238C_1028_4304_9043_09DB76692B74__INCLUDED_)
