#if !defined(AFX_SELECTMPDLG_H__FB232D56_66E0_49E9_BB88_E2ADAD8535EF__INCLUDED_)
#define AFX_SELECTMPDLG_H__FB232D56_66E0_49E9_BB88_E2ADAD8535EF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectMPDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectMPDlg dialog

class CSelectMPDlg : public CDialog
{
// Construction
public:
	CSelectMPDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSelectMPDlg)
	enum { IDD = IDD_SELECT_MP };
	CString	m_strFile;
	BOOL	m_bReplaceEventByName;
	BOOL	m_bReplaceEventByCode;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectMPDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSelectMPDlg)
	afx_msg void OnButtonBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTMPDLG_H__FB232D56_66E0_49E9_BB88_E2ADAD8535EF__INCLUDED_)
