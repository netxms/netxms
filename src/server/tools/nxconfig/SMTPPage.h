#if !defined(AFX_SMTPPAGE_H__96C0DCD6_9849_4299_99B0_3C8BB4397923__INCLUDED_)
#define AFX_SMTPPAGE_H__96C0DCD6_9849_4299_99B0_3C8BB4397923__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SMTPPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSMTPPage dialog

class CSMTPPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSMTPPage)

// Construction
public:
	CSMTPPage();
	~CSMTPPage();

// Dialog Data
	//{{AFX_DATA(CSMTPPage)
	enum { IDD = IDD_SMTP };
	CComboBox	m_wndDrvList;
	CComboBox	m_wndPortList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSMTPPage)
	public:
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSMTPPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeComboSmsdrv();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SMTPPAGE_H__96C0DCD6_9849_4299_99B0_3C8BB4397923__INCLUDED_)
