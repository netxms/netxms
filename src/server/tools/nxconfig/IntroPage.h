#if !defined(AFX_INTROPAGE_H__9E45E6FA_126B_434F_9840_8914430144E6__INCLUDED_)
#define AFX_INTROPAGE_H__9E45E6FA_126B_434F_9840_8914430144E6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// IntroPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIntroPage dialog

class CIntroPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CIntroPage)

// Construction
public:
	CIntroPage();
	~CIntroPage();

// Dialog Data
	//{{AFX_DATA(CIntroPage)
	enum { IDD = IDD_INTRO };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIntroPage)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CIntroPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_INTROPAGE_H__9E45E6FA_126B_434F_9840_8914430144E6__INCLUDED_)
