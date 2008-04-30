#if !defined(AFX_CREATENETMAPDLG_H__67BB72F6_7C22_4898_8E83_CF3F7FE38AFD__INCLUDED_)
#define AFX_CREATENETMAPDLG_H__67BB72F6_7C22_4898_8E83_CF3F7FE38AFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CreateNetMapDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCreateNetMapDlg dialog

class CCreateNetMapDlg : public CDialog
{
// Construction
public:
	DWORD m_dwRootObj;
	CCreateNetMapDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateNetMapDlg)
	enum { IDD = IDD_CREATE_NETMAP };
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateNetMapDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateNetMapDlg)
	afx_msg void OnButtonSelect();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CREATENETMAPDLG_H__67BB72F6_7C22_4898_8E83_CF3F7FE38AFD__INCLUDED_)
