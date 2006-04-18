#if !defined(AFX_SUBMAPBKGNDDLG_H__D2888CBE_9035_481A_A60B_468472ADC497__INCLUDED_)
#define AFX_SUBMAPBKGNDDLG_H__D2888CBE_9035_481A_A60B_468472ADC497__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SubmapBkgndDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSubmapBkgndDlg dialog

class CSubmapBkgndDlg : public CDialog
{
// Construction
public:
	CSubmapBkgndDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSubmapBkgndDlg)
	enum { IDD = IDD_SUBMAP_BKGND };
	CString	m_strFileName;
	int		m_nScaleMethod;
	int		m_nBkType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSubmapBkgndDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSubmapBkgndDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUBMAPBKGNDDLG_H__D2888CBE_9035_481A_A60B_468472ADC497__INCLUDED_)
