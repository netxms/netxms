#if !defined(AFX_POLLCFGPAGE_H__1412427F_51D7_4348_927F_982AE26A3E10__INCLUDED_)
#define AFX_POLLCFGPAGE_H__1412427F_51D7_4348_927F_982AE26A3E10__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PollCfgPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPollCfgPage dialog

class CPollCfgPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPollCfgPage)

// Construction
public:
	CPollCfgPage();
	~CPollCfgPage();

// Dialog Data
	//{{AFX_DATA(CPollCfgPage)
	enum { IDD = IDD_POLLING };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPollCfgPage)
	public:
	virtual LRESULT OnWizardBack();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPollCfgPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckRunDiscovery();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_POLLCFGPAGE_H__1412427F_51D7_4348_927F_982AE26A3E10__INCLUDED_)
