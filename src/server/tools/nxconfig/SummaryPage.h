#if !defined(AFX_SUMMARYPAGE_H__6406880F_F357_452C_A701_D64C46C7D1A9__INCLUDED_)
#define AFX_SUMMARYPAGE_H__6406880F_F357_452C_A701_D64C46C7D1A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SummaryPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSummaryPage dialog

class CSummaryPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CSummaryPage)

// Construction
public:
	CSummaryPage();
	~CSummaryPage();

// Dialog Data
	//{{AFX_DATA(CSummaryPage)
	enum { IDD = IDD_SUMMARY };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSummaryPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSummaryPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SUMMARYPAGE_H__6406880F_F357_452C_A701_D64C46C7D1A9__INCLUDED_)
