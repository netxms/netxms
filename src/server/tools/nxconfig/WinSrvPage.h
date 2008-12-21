#if !defined(AFX_WINSRVPAGE_H__67137572_8E6F_4933_BBEC_645B62288FDF__INCLUDED_)
#define AFX_WINSRVPAGE_H__67137572_8E6F_4933_BBEC_645B62288FDF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WinSrvPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWinSrvPage dialog

class CWinSrvPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CWinSrvPage)

// Construction
public:
	CWinSrvPage();
	~CWinSrvPage();

// Dialog Data
	//{{AFX_DATA(CWinSrvPage)
	enum { IDD = IDD_SERVICE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWinSrvPage)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWinSrvPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioSystem();
	afx_msg void OnRadioUser();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WINSRVPAGE_H__67137572_8E6F_4933_BBEC_645B62288FDF__INCLUDED_)
