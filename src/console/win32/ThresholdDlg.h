#if !defined(AFX_THRESHOLDDLG_H__AD19B0E7_FD78_4AB5_BFDE_324D2F4D2F93__INCLUDED_)
#define AFX_THRESHOLDDLG_H__AD19B0E7_FD78_4AB5_BFDE_324D2F4D2F93__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ThresholdDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CThresholdDlg dialog

class CThresholdDlg : public CDialog
{
// Construction
public:
	CThresholdDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CThresholdDlg)
	enum { IDD = IDD_THRESHOLD };
	CStatic	m_wndStaticSamples;
	CStatic	m_wndStaticFor;
	CEdit	m_wndEditArg1;
	CComboBox	m_wndComboOperation;
	CComboBox	m_wndComboFunction;
	int		m_iFunction;
	CString	m_strValue;
	long	m_dwArg1;
	int		m_iOperation;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CThresholdDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CThresholdDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboFunction();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_THRESHOLDDLG_H__AD19B0E7_FD78_4AB5_BFDE_324D2F4D2F93__INCLUDED_)
