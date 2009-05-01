#if !defined(AFX_TRANSFORMTESTDLG_H__F8ECC60E_9EDC_4434_B8D2_78D6F14B2412__INCLUDED_)
#define AFX_TRANSFORMTESTDLG_H__F8ECC60E_9EDC_4434_B8D2_78D6F14B2412__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TransformTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTransformTestDlg dialog

class CTransformTestDlg : public CDialog
{
// Construction
public:
	DWORD m_dwItemId;
	DWORD m_dwNodeId;
	CString m_strScript;
	CTransformTestDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CTransformTestDlg)
	enum { IDD = IDD_TEST_TRANSFORMATION };
	CStatic	m_wndStatusIcon;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTransformTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTransformTestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonRun();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRANSFORMTESTDLG_H__F8ECC60E_9EDC_4434_B8D2_78D6F14B2412__INCLUDED_)
