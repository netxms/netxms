#if !defined(AFX_LOGGINGPAGE_H__330C7F5B_6859_4E9A_90B0_FDFC07422E30__INCLUDED_)
#define AFX_LOGGINGPAGE_H__330C7F5B_6859_4E9A_90B0_FDFC07422E30__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LoggingPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLoggingPage dialog

class CLoggingPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CLoggingPage)

// Construction
public:
	CLoggingPage();
	~CLoggingPage();

// Dialog Data
	//{{AFX_DATA(CLoggingPage)
	enum { IDD = IDD_LOG_FILE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLoggingPage)
	public:
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLoggingPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioSyslog();
	afx_msg void OnRadioFile();
	afx_msg void OnButtonBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGGINGPAGE_H__330C7F5B_6859_4E9A_90B0_FDFC07422E30__INCLUDED_)
